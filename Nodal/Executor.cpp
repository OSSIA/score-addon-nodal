#include "Executor.hpp"

#include <Process/ExecutionContext.hpp>
#include <Process/ExecutionSetup.hpp>

#include <ossia/dataflow/port.hpp>
#include <ossia/dataflow/node_chain_process.hpp>

#include <Nodal/Process.hpp>
#include <score/document/DocumentContext.hpp>
#include <Process/Execution/ProcessComponent.hpp>
#include <score/application/ApplicationContext.hpp>
#include <ossia/detail/flat_set.hpp>

namespace ossia
{

struct node_graph_process final : public ossia::time_process
{
  node_graph_process()
  {
    m_lastDate = ossia::Zero;
  }

  void state(
      ossia::time_value from, ossia::time_value to, double relative_position,
      ossia::time_value tick_offset, double speed) override
  {
    for (auto& process : processes)
    {
      process->state(from, to, relative_position, tick_offset, speed);
    }
    m_lastDate = to;
  }

  void add_process(std::shared_ptr<ossia::time_process> p, std::shared_ptr<ossia::graph_node>&& n)
  {
    nodes.insert(std::move(n));
    processes.insert(std::move(p));
  }

  void start() override
  {
    for (auto& process : processes)
    {
      process->start();
    }
  }

  void stop() override
  {
    for (auto& process : processes)
    {
      process->stop();
    }
    for (auto& node : nodes)
    {
      node->all_notes_off();
    }
  }

  void pause() override
  {
    for (auto& process : processes)
    {
      process->pause();
    }
  }

  void resume() override
  {
    for (auto& process : processes)
    {
      process->resume();
    }
  }

  void offset(time_value date, double pos) override
  {
    for (auto& process : processes)
    {
      process->offset(date, pos);
    }
    for (auto& node : nodes)
    {
      node->all_notes_off();
    }
  }

  void transport(ossia::time_value date, double pos) override
  {
    for (auto& process : processes)
    {
      process->transport(date, pos);
    }
    for (auto& node : nodes)
    {
      node->all_notes_off();
    }
  }

  void mute_impl(bool b) override
  {
    for (auto& node : nodes)
      node->set_mute(b);
  }
  ossia::flat_set<std::shared_ptr<ossia::graph_node>> nodes;
  ossia::flat_set<std::shared_ptr<ossia::time_process>> processes;
  ossia::time_value m_lastDate{ossia::Infinite};

};
}
namespace Nodal
{


ProcessExecutorComponent::ProcessExecutorComponent(
    Nodal::Model& element, const Execution::Context& ctx,
    const Id<score::Component>& id, QObject* parent)
    : ProcessComponent_T{element, ctx, id, "NodalExecutorComponent", parent}
{
  // TODO load node
  auto p = std::make_shared<ossia::node_graph_process>();
  m_ossia_process = p;

  std::vector<Execution::ExecutionCommand> commands;
  auto& fact = ctx.doc.app.interfaces<Execution::ProcessComponentFactoryList>();

  for(Node& node : element.nodes)
  {
    auto& proc = node.process();
    if(Execution::ProcessComponentFactory* f = fact.factory(proc))
    {
      auto comp = f->make(proc, ctx, Id<score::Component>{proc.id_val()}, this);
      if(comp)
      {
        reg(m_nodes[node.id()] = {comp}, commands);
        auto child_n = comp->node;
        auto child_p = comp->OSSIAProcessPtr();
        if(child_n && child_p)
        {
          p->add_process(std::move(child_p), std::move(child_n));
        }
      }
    }
  }

  in_exec([f = std::move(commands)] {
    for (auto& cmd : f)
      cmd();
  });
}

ProcessExecutorComponent::~ProcessExecutorComponent()
{

}

void ProcessExecutorComponent::unreg(
    const RegisteredNode& fx)
{
  system().setup.unregister_node_soft(
      fx.comp->process().inlets(), fx.comp->process().outlets(), fx.comp->node);
}

void ProcessExecutorComponent::reg(
    const RegisteredNode& fx,
    std::vector<Execution::ExecutionCommand>& vec)
{
  system().setup.register_node(
              fx.comp->process().inlets(), fx.comp->process().outlets(), fx.comp->node, vec);
}

}
