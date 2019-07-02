#include "Process.hpp"

#include <wobjectimpl.h>

W_OBJECT_IMPL(Nodal::Node)
W_OBJECT_IMPL(Nodal::Model)
namespace Nodal
{

Node::Node(std::unique_ptr<Process::ProcessModel> proc,
           const Id<Node>& id, QObject* parent)
  : score::Entity<Node>{id, QStringLiteral("Node"), parent}
  , m_impl{std::move(proc)}
{
  SCORE_ASSERT(m_impl);
  m_impl->setParent(this);
}

Node::~Node()
{

}

QPointF Node::position() const noexcept { return m_position; }

QSizeF Node::size() const noexcept { return m_size; }

Process::ProcessModel& Node::process() const noexcept { return *m_impl; }

void Node::setPosition(const QPointF& v)
{
  if(v != m_position)
  {
    m_position = v;
    positionChanged(v);
  }
}

void Node::setSize(const QSizeF& v)
{
  if(v != m_size)
  {
    m_size = v;
    sizeChanged(v);
  }
}





Model::Model(
    const TimeVal& duration, const Id<Process::ProcessModel>& id,
    QObject* parent)
    : Process::ProcessModel{duration, id, "NodalProcess", parent}
{
  metadata().setInstanceName(*this);
}

Model::~Model()
{
}

QString Model::prettyName() const noexcept
{
  return tr("Nodal Process");
}

void Model::setDurationAndScale(const TimeVal& newDuration) noexcept
{
  for(Node& n : this->nodes)
    n.process().setParentDuration(ExpandMode::Scale, newDuration);
}

void Model::setDurationAndGrow(const TimeVal& newDuration) noexcept
{
  for(Node& n : this->nodes)
    n.process().setParentDuration(ExpandMode::GrowShrink, newDuration);
}

void Model::setDurationAndShrink(const TimeVal& newDuration) noexcept
{
  for(Node& n : this->nodes)
    n.process().setParentDuration(ExpandMode::GrowShrink, newDuration);
}
}
template <>
void DataStreamReader::read(const Nodal::Model& proc)
{
    insertDelimiter();
}

template <>
void DataStreamWriter::write(Nodal::Model& proc)
{
    checkDelimiter();
}

template <>
void JSONObjectReader::read(const Nodal::Model& proc)
{
}

template <>
void JSONObjectWriter::write(Nodal::Model& proc)
{
}
