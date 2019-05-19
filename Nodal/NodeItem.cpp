#include <Nodal/NodeItem.hpp>
#include <Nodal/Process.hpp>
#include <Process/Style/ScenarioStyle.hpp>
#include <Process/ProcessFactory.hpp>
#include <Process/ProcessList.hpp>
#include <score/application/GUIApplicationContext.hpp>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <Control/DefaultEffectItem.hpp>
#include <score/document/DocumentContext.hpp>
#include <Process/Dataflow/PortFactory.hpp>
#include <Process/Dataflow/PortItem.hpp>
#include <Nodal/Commands.hpp>

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
    item->setPos(x, -1.5);
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
    item->setPos(x, boundingRect().height() + 1.5);
    m_outlets.push_back(item);

    x += 10.;
  }
}

NodeItem::NodeItem(const Node& model, const score::DocumentContext& ctx, QGraphicsItem* parent)
  : QGraphicsItem{parent}
  , m_model{model}
  , m_context{ctx}
{
  setAcceptedMouseButtons(Qt::LeftButton);
  setAcceptHoverEvents(true);

  auto& fact = ctx.app.interfaces<Process::LayerFactoryList>();
  if (auto factory = fact.findDefaultFactory(model.process()))
  {
    auto fx = factory->makeItem(model.process(), ctx, this);
    if(fx)
    {
        m_fx = fx;
        connect(fx, &score::ResizeableItem::sizeChanged,
                this, [this] {
           prepareGeometryChange();
           update();
        });
    }
  }

  if (!m_fx)
  {
    m_fx = new Media::Effect::DefaultEffectItem{model.process(), ctx, this};
  }


  resetInlets(model.process());
  resetOutlets(model.process());

  m_fx->setPos(0, 10);
  ::bind(model, Node::p_position{}, this, [this] (QPointF p) {
      if(p != pos())
          setPos(p);
  });

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
  return {0, 0, sz.width(), 10 + sz.height()};
}

void NodeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    auto& style = Process::Style::instance();
    painter->setPen(m_hover ? QPen(Qt::red) : style.RectPen);
    painter->setBrush(style.RectBrush);
    painter->drawRoundedRect(boundingRect(), 2., 2.);
}

void NodeItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  event->accept();
}

void NodeItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  auto origp = mapToItem(parentItem(), event->buttonDownPos(Qt::LeftButton));
  auto p = mapToItem(parentItem(), event->pos());
  m_context.dispatcher.submit<MoveNode>(m_model, m_model.position() + (p - origp));
  event->accept();
}

void NodeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  auto origp = mapToItem(parentItem(), event->buttonDownPos(Qt::LeftButton));
  auto p = mapToItem(parentItem(), event->pos());
  m_context.dispatcher.submit<MoveNode>(m_model, m_model.position() + (p - origp));
  m_context.dispatcher.commit();
  event->accept();
}

void NodeItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    m_hover = true;
    update();
    event->accept();
}

void NodeItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    m_hover = false;
    update();
    event->accept();
}
}
