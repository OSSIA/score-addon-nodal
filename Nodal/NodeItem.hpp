#pragma once
#include <QGraphicsItem>
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
  NodeItem(const Node& model, const score::DocumentContext& ctx, QGraphicsItem* parent);
  const Id<Node>& id() const noexcept;
  ~NodeItem();

private:
  QRectF boundingRect() const override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

  void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
  void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

  void resetInlets(Process::ProcessModel& effect);
  void resetOutlets(Process::ProcessModel& effect);

  const Node& m_model;
  TitleItem* m_title{};
  QGraphicsItem* m_fx{};

  std::vector<Dataflow::PortItem*> m_inlets, m_outlets;
  const score::DocumentContext& m_context;

  bool m_hover{false};
};
}
