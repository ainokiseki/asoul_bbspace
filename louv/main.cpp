#include"graph.h"

int main() {



	//fstream fout(string("rs.txt"), ios::out);
	//vector<graph>v;
	//v.push_back(graph(string("digraph.txt")));
	//for (int i = 0; i < 5; i++) {
	//	v[i].louvain_one_step();
	//	v.push_back(v[i].generate_graph());
	//	v[i + 1].output_graph(string("clustgraph_") + to_string(i + 1) + string(".txt"));
	//}
	//for (auto& j : v[0].nodemap) {
	//	fout << j.first<<" ";
	//	string x = j.first;
	//	for (int i = 0; i < v.size()-1; i++) {
	//		fout << v[i].vnode[v[i].nodemap[x]].clustnum << " ";
	//		x = to_string(v[i].vnode[v[i].nodemap[x]].clustnum);
	//	}
	//	fout << "\n";
	//}
	graph g("clustgraph_5.txt");
	g.louvain_one_step();
	return 0;
}