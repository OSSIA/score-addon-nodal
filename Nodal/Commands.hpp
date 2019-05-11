#pragma once
#include <Nodal/CommandFactory.hpp>
#include <Nodal/Process.hpp>
#include <score/command/PropertyCommand.hpp>

PROPERTY_COMMAND_T(
    Nodal,
    MoveNode,
    Node::p_position,
    "Move node")

namespace Nodal
{
class Model;
class Node;
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




class RemoveNode final : public score::Command
{
  SCORE_COMMAND_DECL(
      CommandFactoryName(),
      RemoveNode,
      "Remove a node")
public:
  RemoveNode(
      const Nodal::Model& p,
      const Nodal::Node& n
      )
  {

  }

private:
  void undo(const score::DocumentContext& ctx) const override
  {
    // TODO save / restore cables

  }
  void redo(const score::DocumentContext& ctx) const override
  {

  }

  void serializeImpl(DataStreamInput&) const override
  {

  }
  void deserializeImpl(DataStreamOutput&) override
  {

  }

  Path<Nodal::Model> m_path;
  Id<Nodal::Node> m_id;
  QByteArray m_block;
};


}
