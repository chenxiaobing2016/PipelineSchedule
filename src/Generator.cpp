#include <vector>
#include <random>

#include "Processor.h"
#include "Generator.h"
using namespace std;

vector<Task> Generator::genTaskDAG() {
  default_random_engine e;
  vector<vector<OpGrid>> DAG(height, vector<OpGrid> (width));
  for (auto h_idx = 0; h_idx < height; h_idx++) {
    for (auto w_idx = 0; w_idx < width; w_idx++) {
      int type = e() % 8 + 2;
      DAG[h_idx][w_idx].ot = (OperationType)type;
      int in_d, out_d;
      switch(type) {
	case OperationType::CONV :
	case OperationType::POOL :
	case OperationType::FC :
	  in_d = 1;
	  out
      }
      
    }
  }
}
