#include "graph.h"
#include "keysmanager/keysmanager.h"

#include <QJsonDocument>
#include <QJsonArray>

#include <boost/graph/graphviz.hpp>

#include <limits>
#include <regex>

namespace util {

std::ostream& operator<< (std::ostream& os, const graph_t& graph)
{
  const auto vertex_writer = [&](std::ostream& os, const descriptor_t& vertex) {
    os << " [label=\""
       << std::regex_replace(boost::get(boost::vertex_bundle, graph)[vertex],
                             std::regex{"\""},"\\\"")
       << "\"]";
  };

  boost::write_graphviz(os, graph, vertex_writer);

  return os;
}

graph_t json_to_graph(const QJsonObject& root)
{
  graph_t graph;

  detail::json_to_graph_impl(root, graph, std::numeric_limits<descriptor_t>::max());

  return graph;
}

} // namespace util


namespace util::detail {


QString to_string(const QJsonValue& value)
{
  if (value.isBool())          return value.toBool() ? "true" : "false";
  else if (value.isString())   return value.toString();
  else if (value.isDouble())   return QString::number(value.toDouble());
  else if (value.isNull())     return "null";
  else if (value.isArray())    return QJsonDocument{value.toArray()}.toJson(QJsonDocument::Compact);
  else  /* value.isObject() */ return QJsonDocument{value.toObject()}.toJson(QJsonDocument::Compact);
}


void json_to_graph_impl(const QJsonObject& obj, graph_t& graph, descriptor_t parent_vertex)
{
  KeysManager km(obj);
  QStringList text;

  for (const auto& key : km.keys_not_array_or_object())
    text << key + ": " + to_string(obj.value(key));

  const auto vertex = boost::add_vertex(vertex_t{text.join("\n").toStdString()}, graph);

  if (parent_vertex < boost::num_vertices(graph))
    boost::add_edge(parent_vertex, vertex, graph);

  for (const auto& child_key : km.keys_is_array_or_object()) {
    const auto child = obj.value(child_key);

    if (child.isArray()) {
      const auto array_key = boost::add_vertex(vertex_t{child_key.toStdString()}, graph);
      boost::add_edge(vertex, array_key, graph);
      json_to_graph_impl(child.toArray(), graph, array_key);
    } else { // is object
      json_to_graph_impl(child.toObject(), graph, vertex);
    }
  }
}


void json_to_graph_impl(const QJsonValue& value, graph_t& graph, descriptor_t parent_vertex)
{
  const auto line = to_string(value);
  const auto vertex = boost::add_vertex(vertex_t{line.toStdString()}, graph);
  boost::add_edge(parent_vertex, vertex, graph);
}


void json_to_graph_impl(const QJsonArray& array, graph_t& graph, descriptor_t parent_vertex)
{
  for (const auto& value : array) {
    if (value.isArray()) {
      const auto array_key = boost::add_vertex(vertex_t{}, graph);
      boost::add_edge(parent_vertex, array_key, graph);
      json_to_graph_impl(value.toArray(), graph, array_key);
    } else if (value.isObject()) {
      json_to_graph_impl(value.toObject(), graph, parent_vertex);
    } else { // isValue
      json_to_graph_impl(QJsonValue{value}, graph, parent_vertex);
    }
  }
}


} // namespace util::detail
