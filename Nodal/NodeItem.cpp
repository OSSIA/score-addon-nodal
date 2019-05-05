#include <Nodal/NodeItem.hpp>
#include <Nodal/Process.hpp>
#include <Process/ProcessFactory.hpp>
#include <Process/ProcessList.hpp>
#include <score/application/GUIApplicationContext.hpp>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <Control/DefaultEffectItem.hpp>
#include <score/document/DocumentContext.hpp>
#include <Process/Dataflow/PortFactory.hpp>
#include <Process/Dataflow/PortItem.hpp>
namespace Nodal
{

void NodeItem::resetInlets(
    Process::ProcessModel& effect)
{
  qDeleteAll(m_inlets);
  m_inlets.clear();
  qreal x = 10;
  auto& portFactory
      = score::GUIAppContext().interfaces<Process::PortFactoryList>();
  for (Process::Inlet* port : effect.inlets())
  {
    if (port->hidden)
      continue;
    Process::PortFactory* fact = portFactory.get(port->concreteKey());
    Dataflow::PortItem* item = fact->makeItem(*port, m_context, this, this);
    item->setPos(x, -5.);
    m_inlets.push_back(item);

    x += 10.;
  }
}

void NodeItem::resetOutlets(
    Process::ProcessModel& effect)
{
  qDeleteAll(m_outlets);
  m_outlets.clear();
  qreal x = 10;
  auto& portFactory
      = score::AppContext().interfaces<Process::PortFactoryList>();
  for (Process::Outlet* port : effect.outlets())
  {
    if (port->hidden)
      continue;
    Process::PortFactory* fact = portFactory.get(port->concreteKey());
    auto item = fact->makeItem(*port, m_context, this, this);
    item->setPos(x, boundingRect().height() - 5.);
    m_outlets.push_back(item);

    x += 10.;
  }
}

NodeItem::NodeItem(const Node& model, const score::DocumentContext& ctx, QGraphicsItem* parent)
  : QGraphicsItem{parent}
  , m_model{model}
  , m_context{ctx}
{
  auto& fact = ctx.app.interfaces<Process::LayerFactoryList>();
  if (auto factory = fact.findDefaultFactory(model.process()))
  {
    m_fx = factory->makeItem(model.process(), ctx, this);
  }

  if (!m_fx)
  {
    m_fx = new Media::Effect::DefaultEffectItem{model.process(), ctx, this};
  }

  resetInlets(model.process());
  resetOutlets(model.process());

}

const Id<Node>& NodeItem::id() const noexcept
{
  return m_model.id();
}

NodeItem::~NodeItem()
{

}

QRectF NodeItem::boundingRect() const
{
  const auto sz = m_fx->boundingRect();
  return {0, 0, sz.width(), sz.height()};
}

void NodeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  painter->drawRoundedRect(boundingRect(), 2., 2.);
}

void NodeItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  event->accept();
}

void NodeItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  event->accept();
}

void NodeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  event->accept();
}
}
