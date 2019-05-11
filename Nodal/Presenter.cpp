#include <score/command/Dispatchers/CommandDispatcher.hpp>
#include <Nodal/CommandFactory.hpp>
#include <Nodal/Presenter.hpp>
#include <Nodal/Process.hpp>
#include <Nodal/View.hpp>
#include <Nodal/Commands.hpp>

#include <Process/ProcessMimeSerialization.hpp>
// TODO use me
template<typename Entities, typename Presenter>
void bind(const Entities& model, Presenter& presenter)
{
  for (const auto& entity : model)
    presenter.on_created(entity);

  model.added.template connect<&Presenter::on_created>(presenter);
  model.removed.template connect<&Presenter::on_removing>(presenter);
}

namespace Nodal
{


Presenter::Presenter(
    const Model& layer, View* view,
    const Process::ProcessPresenterContext& ctx, QObject* parent)
    : Process::LayerPresenter{ctx, parent}, m_model{layer}, m_view{view}
{
  bind(layer.nodes, *this);


  connect(view, &View::dropReceived, this, [&] (const QPointF& pos, const QMimeData& mime) {
    const auto& ctx = context().context;
    if (mime.hasFormat(score::mime::processdata()))
    {
      Mime<Process::ProcessData>::Deserializer des{mime};
      Process::ProcessData p = des.deserialize();

      auto cmd = new CreateNode(layer, pos, p.key, p.customData);
      CommandDispatcher<> d{ctx.commandStack};
      d.submit(cmd);
    }
  });

}
Presenter::~Presenter()
{
  m_nodes.remove_all();
}
void Presenter::setWidth(qreal val)
{
  m_view->setWidth(val);
}

void Presenter::setHeight(qreal val)
{
  m_view->setHeight(val);
}

void Presenter::putToFront()
{
  m_view->setOpacity(1);
}

void Presenter::putBehind()
{
  m_view->setOpacity(0.2);
}

void Presenter::on_zoomRatioChanged(ZoomRatio)
{
}

void Presenter::parentGeometryChanged()
{
}

const Process::ProcessModel& Presenter::model() const
{
  return m_model;
}

const Id<Process::ProcessModel>& Presenter::modelId() const
{
  return m_model.id();
}

void Presenter::on_created(const Node& n)
{
  auto item = new NodeItem{n, m_context.context, m_view};
  item->setPos(n.position());
  m_nodes.insert(item);
}

void Presenter::on_removing(const Node& n)
{
  m_nodes.erase(n.id());
}
}
