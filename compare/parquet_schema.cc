#include <memory>

#include <parquet/schema.h>
#include <parquet/types.h>

std::shared_ptr<parquet::schema::GroupNode>
InitParquetSchema() {
  using namespace parquet;
  using namespace parquet::schema;

  NodeVector fields;
  fields.push_back(PrimitiveNode::Make("B_FlightDistance", Repetition::REQUIRED, Type::DOUBLE));
  fields.push_back(PrimitiveNode::Make("B_VertexChi2", Repetition::REQUIRED, Type::DOUBLE));

  fields.push_back(PrimitiveNode::Make("H1_PX", Repetition::REQUIRED, Type::DOUBLE));
  fields.push_back(PrimitiveNode::Make("H1_PY", Repetition::REQUIRED, Type::DOUBLE));
  fields.push_back(PrimitiveNode::Make("H1_PZ", Repetition::REQUIRED, Type::DOUBLE));
  fields.push_back(PrimitiveNode::Make("H1_ProbK", Repetition::REQUIRED, Type::DOUBLE));
  fields.push_back(PrimitiveNode::Make("H1_ProbPi", Repetition::REQUIRED, Type::DOUBLE));
  fields.push_back(PrimitiveNode::Make("H1_Charge", Repetition::REQUIRED, Type::INT32, ConvertedType::INT_32));
  fields.push_back(PrimitiveNode::Make("H1_isMuon", Repetition::REQUIRED, Type::INT32, ConvertedType::INT_32));
  fields.push_back(PrimitiveNode::Make("H1_IpChi2", Repetition::REQUIRED, Type::DOUBLE));

  fields.push_back(PrimitiveNode::Make("H2_PX", Repetition::REQUIRED, Type::DOUBLE));
  fields.push_back(PrimitiveNode::Make("H2_PY", Repetition::REQUIRED, Type::DOUBLE));
  fields.push_back(PrimitiveNode::Make("H2_PZ", Repetition::REQUIRED, Type::DOUBLE));
  fields.push_back(PrimitiveNode::Make("H2_ProbK", Repetition::REQUIRED, Type::DOUBLE));
  fields.push_back(PrimitiveNode::Make("H2_ProbPi", Repetition::REQUIRED, Type::DOUBLE));
  fields.push_back(PrimitiveNode::Make("H2_Charge", Repetition::REQUIRED, Type::INT32, ConvertedType::INT_32));
  fields.push_back(PrimitiveNode::Make("H2_isMuon", Repetition::REQUIRED, Type::INT32, ConvertedType::INT_32));
  fields.push_back(PrimitiveNode::Make("H2_IpChi2", Repetition::REQUIRED, Type::DOUBLE));

  fields.push_back(PrimitiveNode::Make("H3_PX", Repetition::REQUIRED, Type::DOUBLE));
  fields.push_back(PrimitiveNode::Make("H3_PY", Repetition::REQUIRED, Type::DOUBLE));
  fields.push_back(PrimitiveNode::Make("H3_PZ", Repetition::REQUIRED, Type::DOUBLE));
  fields.push_back(PrimitiveNode::Make("H3_ProbK", Repetition::REQUIRED, Type::DOUBLE));
  fields.push_back(PrimitiveNode::Make("H3_ProbPi", Repetition::REQUIRED, Type::DOUBLE));
  fields.push_back(PrimitiveNode::Make("H3_Charge", Repetition::REQUIRED, Type::INT32, ConvertedType::INT_32));
  fields.push_back(PrimitiveNode::Make("H3_isMuon", Repetition::REQUIRED, Type::INT32, ConvertedType::INT_32));
  fields.push_back(PrimitiveNode::Make("H3_IpChi2", Repetition::REQUIRED, Type::DOUBLE));

  return std::static_pointer_cast<GroupNode>(
      GroupNode::Make("event", Repetition::REQUIRED, fields));
}
