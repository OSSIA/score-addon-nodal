#pragma once
#include <Process/Execution/ProcessComponent.hpp>

#include <ossia/dataflow/node_process.hpp>
#include <ossia/detail/hash_map.hpp>
namespace Nodal
{
class Model;
class ProcessExecutorComponent final
    : public Execution::ProcessComponent_T<
          Nodal::Model, ossia::node_process>
{
  COMPONENT_METADATA("e85e0114-2a7e-4569-8a1d-f00c9fd22960")
public:
  ProcessExecutorComponent(
      Model& element, const Execution::Context& ctx,
      const Id<score::Component>& id, QObject* parent);

  struct RegisteredNode
  {
    std::shared_ptr<Execution::ProcessComponent> comp;
  };

  ossia::fast_hash_map<Id<Process::ProcessModel>, RegisteredNode> m_nodes;
};

using ProcessExecutorComponentFactory
    = Execution::ProcessComponentFactory_T<ProcessExecutorComponent>;
}
