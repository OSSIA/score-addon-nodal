#include "Executor.hpp"

#include <Process/ExecutionContext.hpp>

#include <ossia/dataflow/port.hpp>

#include <Nodal/Process.hpp>
namespace Nodal
{
class node final : public ossia::nonowning_graph_node
{
public:
  node()
  {
  }

  void
  run(ossia::token_request tk, ossia::exec_state_facade s) noexcept override
  {
  }

  std::string label() const noexcept override
  {
    return "nodal";
  }

private:
};

ProcessExecutorComponent::ProcessExecutorComponent(
    Nodal::Model& element, const Execution::Context& ctx,
    const Id<score::Component>& id, QObject* parent)
    : ProcessComponent_T{element, ctx, id, "NodalExecutorComponent", parent}
{
  auto n = std::make_shared<Nodal::node>();
  this->node = n;
  m_ossia_process = std::make_shared<ossia::node_process>(n);

  /** Don't forget that the node executes in another thread.
   * -> handle live updates with the in_exec function, e.g. :
   *
   * connect(&element.metadata(), &score::ModelMetadata::ColorChanged,
   *         this, [=] (const QColor& c) {
   *
   *   in_exec([c,n=std::dynamic_pointer_cast<Nodal::node>(this->node)] {
   *     n->set_color(c);
   *   });
   *
   * });
   */
}
}
