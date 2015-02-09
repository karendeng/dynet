#ifndef CNN_CNN_H_
#define CNN_CNN_H_

#include <string>
#include <vector>
#include <iostream>
#include <initializer_list>
#include <Eigen/Eigen>

// Computation graph where nodes represent forward and backward intermediate
// values, and edges represent functions of multiple values. To represent the
// fact that a function may have multiple arguments, edges have a single head
// and 0, 1, 2, or more tails. (Constants, inputs, and parameters are
// represented as functions of 0 parameters.)
// Example: given the function z = f(x, y), z, x, and y are nodes, and there
// is an edge representing f with which points to the z node (i.e., its head),
// and x and y are the tails of the edge.

namespace cnn {

typedef Eigen::MatrixXd Matrix;
typedef double real;

// TODO pull fx and dEdf out of the Node object and have them
// as local tables in forward/backward algorithms

struct Dim {
  Dim() : rows(1), cols(1) {}
  explicit Dim(unsigned m) : rows(m), cols(1) {}
  Dim(unsigned m, unsigned n) : rows(m), cols(n) {}
  unsigned short rows;
  unsigned short cols;
  Dim transpose() const { return Dim(cols,rows); }
};

inline Dim operator*(const Dim& a, const Dim& b) {
  assert(a.cols == b.rows);
  return Dim(a.rows, b.cols);
}

inline std::ostream& operator<<(std::ostream& os, const Dim& d) {
  return os << '(' << d.rows << ',' << d.cols << ')';
}

inline Matrix Zero(const Dim& d) { return Matrix::Zero(d.rows, d.cols); }
inline Matrix Random(const Dim& d) { return Matrix::Random(d.rows, d.cols) * 0.08; }

struct Edge;
struct Node;

struct Hypergraph {
  ~Hypergraph();
  // construct a graph
  unsigned add_parameter(const Dim& d, const std::string& name = "");
  unsigned add_input(const Dim& d, const std::string& name = "");
  template <class Function> inline unsigned add_function(const std::initializer_list<unsigned>& arguments, const std::string& name = "");

  // perform computations
  Matrix forward();
  void backward();

  // debugging
  void PrintGraphviz() const;

  // data
  std::vector<Edge*> edges;
  std::vector<Node*> nodes;  // **stored in topological order**
};

// represents an SSA variable
// * in_edge is the index of the function that computes the variable
// * out_edges are the list of functions that use this variable
// * f is the computed value of the variable (TODO: remove this, see note below)
// * dEdf is the derivative of the output with respect to the function
struct Node {
  // name is currently just used for debugging- maybe eventually for code
  // generation though
  Node(unsigned in_edge_index, const std::string& name) :
      in_edge(in_edge_index),
      var_name(name) {}

  // dependency structure
  unsigned in_edge;
  std::vector<unsigned> out_edges;

  // debugging
  const std::string& variable_name() const { return var_name; }
  std::string var_name;

  // computation
  // TODO remove these from here, they should be local to the forward/backward
  // algorithms
  Matrix f;               // f(x_1 , ... , x_n)
  Matrix dEdf;            // dE/df
};

// represents a function of zero or more input variables
// functions with zero inputs are constants or optimizeable parameters
struct Edge {
  virtual ~Edge();
  // debugging
  virtual std::string as_string(const std::vector<std::string>& var_names) const = 0;

  // computation
  virtual Matrix forward(const std::vector<const Matrix*>& xs) const = 0;
  // computes the derivative of E with respect to the ith argument to f, that is, xs[i]
  virtual Matrix backward(const std::vector<const Matrix*>& xs,
                          const Matrix& fx,
                          const Matrix& dEdf,
                          unsigned i) const = 0;
  virtual bool has_parameters() const;

  // number of arguments to the function
  inline unsigned arity() const { return tail.size(); }

  // structure
  unsigned head_node;   // index of node to contain result of f
  std::vector<unsigned> tail;
};

template <class Function>
inline unsigned Hypergraph::add_function(const std::initializer_list<unsigned>& arguments, const std::string& name) {
  unsigned new_node_index = nodes.size();
  unsigned new_edge_index = edges.size();
  nodes.push_back(new Node(new_edge_index, name));
  Edge* new_edge = new Function;
  edges.push_back(new_edge);
  new_edge->head_node = new_node_index;
  for (auto ni : arguments) {
    new_edge->tail.push_back(ni);
    nodes[ni]->out_edges.push_back(new_edge_index);
  }
  return new_node_index;
}

}

#endif