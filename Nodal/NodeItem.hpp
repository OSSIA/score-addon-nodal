#pragma once
#include <QGraphicsItem>
#include <Process/ZoomHelper.hpp>
#include <score/model/Identifier.hpp>
namespace score
{
struct DocumentContext;
}
namespace Dataflow {
class PortItem;
}
namespace Process
{
class ProcessModel;
class LayerPresenter;
struct LayerContext;
}

namespace Nodal
{
class TitleItem;
class Node;
class NodeItem
    : public QObject
    , public QGraphicsItem
{
public:
  NodeItem(const Node& model, const Process::LayerContext& ctx, QGraphicsItem* parent);
  const Id<Node>& id() const noexcept;
  ~NodeItem();

  void setZoomRatio(ZoomRatio r);
  void setPlayPercentage(float f);

  void setSelected(bool s);
  const qreal width() const noexcept { return m_size.width(); }

private:
  void setSize(QSizeF sz);
  QRectF boundingRect() const override;
  bool isInSelectionCorner(QPointF f, QRectF r) const;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

  void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
  void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
  void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

  void resetInlets(Process::ProcessModel& effect);
  void resetOutlets(Process::ProcessModel& effect);

  const Node& m_model;
  TitleItem* m_title{};
  QGraphicsItem* m_fx{};
  Process::LayerPresenter* m_presenter{};
  QSizeF m_size{};

  std::vector<Dataflow::PortItem*> m_inlets, m_outlets;
  const Process::LayerContext& m_context;

  ZoomRatio m_ratio{1.};
  bool m_hover{false};
  bool m_selected{false};
};
}
