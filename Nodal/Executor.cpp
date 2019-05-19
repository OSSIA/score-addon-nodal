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
    const ossia::token_request tk{from, to, relative_position, tick_offset,
                                  speed};
    for (auto& node : nodes)
    {
      node->request(tk);
    }
    m_lastDate = to;
  }

  void add_node(std::shared_ptr<ossia::graph_node> n)
  {
    // n->set_prev_date(this->m_lastDate);
    nodes.insert(std::move(n));
  }

  void stop() override
  {
    for (auto& node : nodes)
    {
      node->all_notes_off();
    }
  }

  void offset(time_value date, double pos) override
  {
    for (auto& node : nodes)
    {
      node->all_notes_off();
    }
  }

  void transport(ossia::time_value date, double pos) override
  {
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

        reg(m_nodes[proc.id()] = {comp}, commands);
        if(auto n = comp->node)
        {
          p->add_node(n);
        }
      }
    }
  }

  in_exec([f = std::move(commands)] {
    for (auto& cmd : f)
      cmd();
  });
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
