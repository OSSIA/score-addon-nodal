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
#include <Effect/EffectLayer.hpp>
#include <Process/Dataflow/PortItem.hpp>
#include <Nodal/Commands.hpp>
#include <score/selection/SelectionDispatcher.hpp>
#include <score/graphics/TextItem.hpp>
#include <wobjectimpl.h>
namespace Nodal
{

class TitleItem final
    : public QObject
    , public QGraphicsItem
{
  W_OBJECT(TitleItem)
public:
  TitleItem(
      Process::ProcessModel& effect,
      const score::DocumentContext& ctx,
      NodeItem& item,
      QObject* parent)
  : QObject{parent}
  , m_effect{effect}
  , m_width{170}
  , m_item{item}
  {
    const auto& skin = Process::Style::instance();

    if (auto ui_btn
        = Process::makeExternalUIButton(effect, ctx, this, this))
      ui_btn->setPos({5, 0});

    auto label = new score::SimpleTextItem{skin.IntervalBase, this};
    label->setText(effect.prettyShortName());
    label->setFont(skin.skin.Medium10Pt);
    label->setPos({30, 0});
  }

  QRectF boundingRect() const override { return {0, 0, m_width, 15}; }
  void paint(
      QPainter* painter,
      const QStyleOptionGraphicsItem* option,
      QWidget* widget) override
  {
    static const auto pen = QPen{Qt::transparent};
    auto& style = Process::Style::instance();

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(pen);
    painter->setBrush(m_hover ? QBrush(style.RectPen.color().lighter(110)) : style.RectPen.brush());
    painter->drawRoundedRect(boundingRect(), 2, 2);
    painter->setRenderHint(QPainter::Antialiasing, false);
  }

  void setWidth(qreal w) {
    prepareGeometryChange();
    m_width = w;
  }
  void setHighlight(bool b) {}
  void clicked() W_SIGNAL(clicked)


  void setHover(bool b)
  {
    m_hover = b;
    update();
  }

private:
  const Process::ProcessModel& m_effect;
  NodeItem& m_item;
  qreal m_width{};
  bool m_hover{};
};


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

  // Title
  m_title = new TitleItem{model.process(), ctx, *this, this};
  m_title->setParentItem(this);

  //
  //connect(this, &TitleItem::clicked, this, [&] {
  //  doc.focusDispatcher.focus(&ctx.presenter);
  //  score::SelectionDispatcher{doc.selectionStack}.setAndCommit({&effect});
  //});
  //
  // Body
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
           m_title->setWidth(m_fx->boundingRect().width());
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

  m_fx->setPos({0, m_title->boundingRect().height()});

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
  const auto tsz = m_title->boundingRect();
  return {0, 0, sz.width(), tsz.height() + sz.height()};
}

void NodeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    auto& style = Process::Style::instance();
    painter->setPen(m_hover ? QPen(style.RectPen.color().lighter(110)) : style.RectPen);
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
    m_title->setHover(true);
    update();
    event->accept();
}

void NodeItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    m_hover = false;
    m_title->setHover(false);
    update();
    event->accept();
}
}
W_OBJECT_IMPL(Nodal::TitleItem)
