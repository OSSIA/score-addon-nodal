#include <score/command/Dispatchers/CommandDispatcher.hpp>
#include <Nodal/CommandFactory.hpp>
#include <Nodal/Presenter.hpp>
#include <Nodal/Process.hpp>
#include <Nodal/View.hpp>
#include <score/tools/IdentifierGeneration.hpp>
#include <score/model/path/PathSerialization.hpp>
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



class CreateNode final : public score::Command
{
  SCORE_COMMAND_DECL(
      CommandFactoryName(),
      CreateNode,
      "Create a node")
public:
  CreateNode(
      const Nodal::Model& process,
      QPointF position,
      const UuidKey<Process::ProcessModel>& uuid,
      const QString& dat);

  void undo(const score::DocumentContext& ctx) const override;
  void redo(const score::DocumentContext& ctx) const override;

protected:
  void serializeImpl(DataStreamInput&) const override;
  void deserializeImpl(DataStreamOutput&) override;

private:
  Path<Nodal::Model> m_path;
  QPointF m_pos;
  UuidKey<Process::ProcessModel> m_uuid;
  QString m_data;

  Id<Nodal::Node> m_createdNodeId;
};


CreateNode::CreateNode(
    const Nodal::Model& nodal,
    QPointF position,
    const UuidKey<Process::ProcessModel>& process,
    const QString& dat)
    : m_path{nodal}
    , m_pos{position}
    , m_uuid{process}
    , m_data{dat}
    , m_createdNodeId{getStrongId(nodal.nodes)}
{
}

void CreateNode::undo(const score::DocumentContext& ctx) const
{
  auto& nodal = m_path.find(ctx);
  nodal.nodes.remove(m_createdNodeId);
}

void CreateNode::redo(const score::DocumentContext& ctx) const
{
  auto& nodal = m_path.find(ctx);
  auto fac
      = ctx.app.interfaces<Process::ProcessFactoryList>().get(m_uuid);
  SCORE_ASSERT(fac);
  auto proc = std::unique_ptr<Process::ProcessModel>{fac->make(
        TimeVal{},
        m_data,
        Id<Process::ProcessModel>{},
        nullptr)};

  SCORE_ASSERT(proc);
  // todo handle these asserts
  auto node = new Node{std::move(proc), m_createdNodeId, &nodal};
  node->setSize({100, 100});
  node->setPosition(m_pos);
  nodal.nodes.add(node);
}

void CreateNode::serializeImpl(DataStreamInput& s) const
{
  s << m_path << m_pos << m_uuid << m_data << m_createdNodeId;
}

void CreateNode::deserializeImpl(DataStreamOutput& s)
{
  s >> m_path >> m_pos >> m_uuid >> m_data >> m_createdNodeId;
}

/*
class RemoveNode final : public score::Command
{
  SCORE_COMMAND_DECL(
      ScenarioCommandFactoryName(),
      RemoveNode,
      "Remove a comment block")
public:
  RemoveNode(
      const Scenario::ProcessModel& sc,
      const Scenario::NodeModel& cb
      );

  void undo(const score::DocumentContext& ctx) const override;
  void redo(const score::DocumentContext& ctx) const override;

protected:
  void serializeImpl(DataStreamInput&) const override;
  void deserializeImpl(DataStreamOutput&) override;

private:
  Path<ProcessModel> m_path;
  Id<NodeModel> m_id;
  QByteArray m_block;
};
*/
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
