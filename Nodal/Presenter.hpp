#pragma once
#include <Nodal/Process.hpp>
#include <Nodal/NodeItem.hpp>

#include <Process/Focus/FocusDispatcher.hpp>
#include <Process/LayerPresenter.hpp>
#include <Process/ZoomHelper.hpp>

#include <score/model/Identifier.hpp>
namespace Nodal
{
class Node;
class Model;
class View;
class Presenter final
    : public Process::LayerPresenter
    , public Nano::Observer
{
public:
  using Node = Nodal::Node;

  explicit Presenter(
      const Model& model, View* view,
      const Process::ProcessPresenterContext& ctx, QObject* parent);
  ~Presenter();

  void setWidth(qreal width, qreal defaultWidth) override;
  void setHeight(qreal height) override;

  void putToFront() override;
  void putBehind() override;

  void on_zoomRatioChanged(ZoomRatio) override;

  void parentGeometryChanged() override;

  const Process::ProcessModel& model() const override;
  const Id<Process::ProcessModel>& modelId() const override;

  void on_created(const Node& n);
  void on_removing(const Node& n);

private:
  IdContainer<NodeItem, Node> m_nodes;

  const Model& m_model;
  qreal m_defaultW{};
  ZoomRatio m_ratio{1.};
  View* m_view{};
  QMetaObject::Connection m_con;
};
}
