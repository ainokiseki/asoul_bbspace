#pragma once

#include<vector>
#include<unordered_map>
#include<string>
#include<unordered_set>
#include<random>
#include<tuple>
#include<concurrent_priority_queue.h>
#include<iostream>
#include<fstream>
using namespace std;

using pq = concurrency::concurrent_priority_queue < pair<float, int>> ;

class graph
{
public:
	graph(string filename) {
		total_weight = 0;
		ifstream gfile(filename);
		string a, b; float w;
		while (gfile >> a >> b >> w) {
			if (nodemap.find(a) == nodemap.end()) {
				nodemap[a] = nodemap.size();
				g.push_back(unordered_map<int, float>());
				inlist.push_back(unordered_set<int>());
				node_total_in.push_back(0);
				node_total_out.push_back(0);
				clusterlist.push_back(nodemap[a]);
				clustermap[nodemap[a]] = cluster();
			}
			if (nodemap.find(b) == nodemap.end()) {
				nodemap[b] = nodemap.size();
				g.push_back(unordered_map<int, float>());
				inlist.push_back(unordered_set<int>());
				node_total_in.push_back(0);
				node_total_out.push_back(0);
				clusterlist.push_back(nodemap[b]);
				clustermap[nodemap[b]] = cluster();
			}
			g[nodemap[a]][nodemap[b]] = w;
			inlist[nodemap[b]].insert(nodemap[a]);
			node_total_in[nodemap[b]] += w;
			node_total_out[nodemap[a]] += w;
			total_weight += w;

			//init cluster edge
			clustermap[clusterlist[nodemap[a]]].adj[clusterlist[nodemap[b]]] = std::pair<float, int>(w, 1);
			clustermap[clusterlist[nodemap[a]]].total_out += w;
			clustermap[clusterlist[nodemap[b]]].total_in += w;

		}
		nodenum = g.size();

		dre.seed(seed);



	}
	~graph()=default;



	class cluster
	{
	public:
		cluster() {
			total_in = total_out = 0;
			count = 1;
		}
		~cluster()=default;
		unordered_map<int, pair<float, int>>adj;
		int count;
		float total_in;
		float total_out;
	private:
	};

	int seed = 114514;
	int nodenum;
	float total_weight;


	void remove_cluster_edge(int x, int y,float weight) {
		auto& clust_x = clustermap[x];
		auto& clust_y = clustermap[y];
		auto& clustedge = clust_x.adj[y];
		if (clustedge.second == 1) {
			clust_x.adj.erase(y);
		}
		else {
			clustedge.second--;
			clustedge.first -= weight;
		}
		clust_x.total_out -= weight;
		clust_y.total_in -= weight;
	}


	void remove_point(int clustnum,int &nodenum) {
		//cout << "remove_point:" << clustnum << " " << nodenum << endl;
		auto& clust_x = clustermap[clustnum];

		//delete ingoing edges
		for (auto& i : inlist[nodenum]) {
			int a = clusterlist[i];
			remove_cluster_edge(a, clustnum, g[i][nodenum]);
		}


		if (clust_x.count == 1) {
			//delete cluster
			clustermap.erase(clustnum);
		}
		else {
			//delete outgoing edges
			for (auto& i : g[nodenum]) {

				int a = i.first;

				remove_cluster_edge(clustnum, clusterlist[a], i.second);
			}
			clust_x.count--;
		}
		
		clusterlist[nodenum] = -1;
	}
	void add_cluster_edge(int clustnum1, int clustnum2, float weight) {
		auto& clust1 = clustermap[clustnum1];
		auto& clust2 = clustermap[clustnum2];
		if (clust1.adj.find(clustnum2) == clust1.adj.end()) {
			clust1.adj[clustnum2]=std::pair<float, int>(weight, 1);
		}
		else {
			clust1.adj[clustnum2].first += weight;
			clust1.adj[clustnum2].second++;
		}
		clust1.total_out += weight;
		clust2.total_in += weight;

	}

	void add_point(int clustnum, int& nodenum) {
		//cout << "add_point:" << clustnum << " " << nodenum << endl;
		auto& cluster_x = clustermap[clustnum];
		//add outgoing edges
		for (auto& i : g[nodenum]) {
			int a = i.first;
			add_cluster_edge(clustnum, clusterlist[a], i.second);
		}
		//add ingoing edges
		for (auto&i: inlist[nodenum]) {
			float weight = g[i][nodenum];
			add_cluster_edge(clusterlist[i], clustnum,weight );
		}
		cluster_x.count++;
		//update clusterlist
		clusterlist[nodenum] = clustnum;
	}

	void calculate_community(int nodenum,pq& q) {

		for (auto& i : clustermap) {
			if (i.first == clusterlist[nodenum])continue;//do not test the cluster nodenum belongs to
			int clustnum = i.first;
			auto& clust_x = clustermap[clustnum];
			//calculate d^C_i
			double dic = 0;
			for (auto& j : g[nodenum]) {
				if (clusterlist[j.first] == clustnum) {
					dic += j.second;
				}
			}
			float delta_q = dic - (node_total_out[nodenum]* clust_x.total_in+node_total_in[nodenum]*clust_x.total_out) / total_weight;
			if (delta_q > 0) {
				q.push(pair<float, int>(delta_q, clustnum));
			}

		}
		

	}

	void louvain_one_step() {
		vector<int>v(nodenum);
		for (int i = 0; i < nodenum; i++)v[i] = i;
		shuffle(v.begin(), v.end(), dre);
		while (true) {


			bool flg = false;
			for (auto& i : v) {
				pq q;
				calculate_community(i, q);
				if (!q.empty()) {
					pair<float, int>x;
					q.try_pop(x);
					int j = x.second;
					remove_point(clusterlist[i], i);
					add_point(j, i);
					flg = true;
				}

			}
			if (!flg) {
				return;
			}
		}
	}
	void louvain() {
		print_graph_detail();
		print_cluster_detail();
		louvain_one_step();
		print_cluster();

	}
	void print_cluster() {
		for (auto& i : clustermap) {
			cout << i.first << ":\n";
			for (int j = 0; j < clusterlist.size(); j++) {
				if (clusterlist[j] == i.first) {
					cout << j << " ";
				}
			}
			cout << endl;
		}
	}
	void print_cluster_detail() {
		for (auto& i : clustermap) {
			cout << i.first << ":\n";
			cout << "indu:" << i.second.total_in << " outdu:" << i.second.total_out << endl;
			cout << "adj:";
			for (auto& j : i.second.adj) {
				cout << j.first << " ";
			}
			cout << "\n" << endl;
		}
	}
	void print_graph_detail() {
		cout << "ttl:" << total_weight << endl;
	}
	private:
		unordered_map<string, int>nodemap;
		vector<unordered_map<int, float> >g;
		vector<unordered_set<int>>inlist;
		vector<int>clusterlist;
		unordered_map<int, cluster>clustermap;
		default_random_engine dre;
		vector<float>node_total_in;
		vector<float>node_total_out;

};
