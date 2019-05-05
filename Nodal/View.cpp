#include "View.hpp"

#include <Process/Style/ScenarioStyle.hpp>

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
namespace Nodal
{

View::View(QGraphicsItem* parent) : LayerView{parent}
{
}

View::~View()
{
}

void View::paint_impl(QPainter* painter) const
{
  painter->drawText(boundingRect(), "Change me");
}


void View::dragEnterEvent(QGraphicsSceneDragDropEvent* event)
{
}

void View::dragLeaveEvent(QGraphicsSceneDragDropEvent* event)
{
}

void View::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
{
}

void View::dropEvent(QGraphicsSceneDragDropEvent* event)
{
  dropReceived(event->pos(), *event->mimeData());
}


}
