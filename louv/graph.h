#pragma once

#include<vector>
#include<unordered_map>
#include<string>
#include<unordered_set>
#include<random>
#include<tuple>
#include<concurrent_priority_queue.h>
#include<concurrent_unordered_set.h>
#include<concurrent_queue.h>
#include<iostream>
#include<fstream>
#include<thread>
#include"mytime.h"
using namespace std;

using pq = concurrency::concurrent_priority_queue < pair<float, int>> ;
using cq = concurrency::concurrent_queue<int>;
class graph
{
public:
	double gamma = 2;
	class cluster
	{
	public:
		cluster() {
			total_in = total_out = 0;
			count = 1;
		}
		~cluster() = default;

		int count;
		double total_in;
		double total_out;
	private:
	};

	class node {
	public:
		node(string s,int x) :nodename(s),nodenum(x) {
			clustnum = x;
			total_in = total_out = 0;
		};
		void add_edge(int x, double w) {
			if (adj.find(x) == adj.end())adj[x] = 0;
			adj[x] += w;
			total_out += w;
		}
		string nodename;
		int nodenum;
		int clustnum;
		double total_in;
		double total_out;
		unordered_set<int>inlist;
		unordered_map<int, double>adj;
	};
	class mission {
	public:
		mission(int x, int y, double z) :nodenum(x), clustnum(y), limit(z) {};
		int nodenum;
		int clustnum;
		double limit;
	};
public:
	int seed = 114514;
	int total_nodenum;
	float total_weight;
	vector<node>vnode;
	unordered_map<int, cluster>clustermap;
	default_random_engine dre;
	unordered_map<string, int>nodemap;

public:

	graph() {
		init();
	}

	graph(string filename) {
		init();
		ifstream gfile(filename);
		string a, b; double w;
		while (gfile >> a >> b >> w) {
			if (nodemap.find(a) == nodemap.end()) {
				int x = nodemap.size();
				nodemap[a] = x;
				vnode.push_back(node(a,x));


			}
			if (nodemap.find(b) == nodemap.end()) {
				int x = nodemap.size();
				nodemap[b] = x;
				vnode.push_back(node(b, x));


			}
			int na = nodemap[a]; int nb = nodemap[b];
			vnode[na].adj[nb] = w;
			vnode[nb].inlist.insert(na);
			vnode[na].total_out += w;
			vnode[nb].total_in += w;
			total_weight += w;
			//init cluster edge

		}
		init_clust();

		



	}
	~graph()=default;
	void init() {
		total_weight = 0;
		dre.seed(seed);
	}
	void init_clust() {
		total_nodenum = vnode.size();
		for (int i = 0; i < total_nodenum; i++) {
			clustermap[i] = cluster();
			clustermap[i].total_in = vnode[i].total_in;
			clustermap[i].total_out = vnode[i].total_out;
		}
	}

	void output_graph(string s,int mode=0) {
		
		fstream fout;
		if (mode == 0) {
			fout = fstream(s, ios::out);
		}
		else {
			fout = fstream(s, ios::out|ios::binary);
		}
		for (auto& i : vnode) {
			for (auto& j : i.adj) {
				fout << i.nodename << " " << vnode[j.first].nodename << " " << j.second << endl;
			}
		}


	}

	void remove_point(int clustnum,int &nodenum) {
		//cout << "remove_point:" << clustnum << " " << nodenum << endl;
		auto& clust_x = clustermap[clustnum];
		clust_x.count--;
		//delete ingoing edges
		if (clust_x.count == 0) {
			//delete cluster
			clustermap.erase(clustnum);
		}
		else {
			//delete outgoing edges
			clust_x.total_in -= vnode[nodenum].total_in;
			clust_x.total_out -= vnode[nodenum].total_out;
		}

		vnode[nodenum].clustnum = -1;
	}

	void add_point(int clustnum, int& nodenum) {
		//cout << "add_point:" << clustnum << " " << nodenum << endl;
		auto& cluster_x = clustermap[clustnum];
		//add outgoing edges

		cluster_x.count++;
		//update clusterlist
		vnode[nodenum].clustnum = clustnum;

		cluster_x.total_in += vnode[nodenum].total_in;
		cluster_x.total_out += vnode[nodenum].total_out;

	}
	double calculate_self(int nodenum) {

		int clustnum = vnode[nodenum].clustnum;
		auto& clust_x = clustermap[clustnum];
		double dic = 0;
		double  outadd = 0;
		for (auto& j : vnode[nodenum].adj) {
			if (vnode[j.first].clustnum == clustnum) {
				dic += j.second;
				
			}
		}

		for (auto& j : vnode[nodenum].inlist) {
			if (vnode[j].clustnum == clustnum) {
				dic += vnode[j].adj[nodenum];
			}
		}


		double delta_q = dic - gamma*( vnode[nodenum].total_out * (clust_x.total_in- vnode[nodenum].total_in) + vnode[nodenum].total_in * (clust_x.total_out - vnode[nodenum].total_out)) / total_weight;
		return delta_q;
	}
	void calculate_community_thread(pq& q, atomic<int>& ant,concurrency::concurrent_queue<mission>&work_queue) {
		


		while (true) {


			
			mission x(-1, 0, 0.0);
			while(x.nodenum==-1) work_queue.try_pop(x);
			if (x.nodenum == -2)return;
			int clustnum = x.clustnum;
			int nodenum = x.nodenum;
			double bst = x.limit;


			auto& clust_x = clustermap[clustnum];
			//calculate d^C_i
			double dic = 0;
			for (auto& j : vnode[nodenum].adj) {
				if (vnode[j.first].clustnum == clustnum) {
					dic += j.second;
				}
			}
			double delta_q = dic - (vnode[nodenum].total_out * clust_x.total_in + vnode[nodenum].total_in * clust_x.total_out) / total_weight;
			if (delta_q > bst) {
				q.push(pair<float, int>(delta_q, clustnum));
			}
			ant++;

		}
		
	}


	void calculate_community(int nodenum,pq& q,cq &work_queue,double bst) {

		while (!work_queue.empty()) {
			int clustnum = -1;
			while(clustnum==-1)
			work_queue.try_pop(clustnum);
			


			auto& clust_x = clustermap[clustnum];
			//calculate d^C_i
			double dic = 0;
			for (auto& j : vnode[nodenum].adj) {
				if (vnode[j.first].clustnum == clustnum) {
					dic += j.second;
				}
			}
			for (auto& j : vnode[nodenum].inlist) {
				if (vnode[j].clustnum == clustnum) {
					dic += vnode[j].adj[nodenum];
				}
			}



			double delta_q = dic - gamma*(vnode[nodenum].total_out * clust_x.total_in + vnode[nodenum].total_in * clust_x.total_out) / total_weight;
			if (delta_q > bst) {
				q.push(pair<float, int>(delta_q, clustnum));
			}

		}
		

	}
	void louvain_one_step_thread() {
		int pnum = 2;
		vector<thread>vt;
		atomic<int>ant(0);
		concurrency::concurrent_queue<mission> work_queue;
		pq q;
		for (int i = 0; i < pnum; i++) {
			vt.push_back(thread(&graph::calculate_community_thread, this, std::ref(q), ref(ant), ref(work_queue)));
		}



		vector<int>v(total_nodenum);
		for (int i = 0; i < total_nodenum; i++)v[i] = i;
		shuffle(v.begin(), v.end(), dre);
		timer ava;
		while (true) {
			ava.tic();
			cout << "one step:" << clustermap.size();
			bool flg = false;
			for (auto& i : v) {

				double bst = max(calculate_self(i), 0.0);
				int cnum = vnode[i].clustnum;
				unordered_set<int> clustset;
				for (auto& j : vnode[i].adj) {

					clustset.insert(vnode[j.first].clustnum);

				}
				if (clustset.find(cnum) != clustset.end())clustset.erase(cnum);
				
				for (auto& k : clustset)work_queue.push(mission(i,k,bst));
				while (ant != clustset.size()) {};
				ant = 0;
				if (!q.empty()) {
					pair<float, int>x;
					q.try_pop(x);
					int j = x.second;
					remove_point(vnode[i].clustnum, i);
					add_point(j, i);
					flg = true;
					q.clear();
				}


			}
			if (!flg) {

				for (int i = 0; i < pnum; i++) {
					work_queue.push(mission(-2, -2, 0.0));
				}



				return;
			}
			ava.toc();
			cout << " " << ava.blink << endl;
		}
	}
	void louvain_one_step() {
		cout << "mdlrty:" << calc_total_modularity() << " " << calc_total_modularity2() << endl;
		vector<int>v(total_nodenum);
		for (int i = 0; i < total_nodenum; i++)v[i] = i;
		
		pq q;
		timer ava;
		while (true) {
			shuffle(v.begin(), v.end(), dre);
			//for (int i = 0; i < 5; i++)cout << v[i] << " "; cout << endl;
			ava.tic();
			cout << "one step:" << clustermap.size();
			bool flg = false;
		
			for (auto& i : v) {
				
				double bst = max(calculate_self(i),0.0);
				int cnum = vnode[i].clustnum;
				unordered_set<int> clustset;
				for (auto& j : vnode[i].adj) {
					
						clustset.insert(vnode[j.first].clustnum);
					
				}
				if (clustset.find(cnum) != clustset.end())clustset.erase(cnum);
				cq work_queue;
				for (auto& k : clustset)work_queue.push(k);

				calculate_community(i, q, work_queue,bst);


				if (!q.empty()) {
					pair<float, int>x;
					q.try_pop(x);
					int j = x.second;
					remove_point(vnode[i].clustnum, i);
					add_point(j, i);
					flg = true;
					q.clear();
				}

			}
			if (!flg) {
				return;
			}
			ava.toc();
			cout << " " << ava.blink<<" "<<calc_total_modularity() <<" "<< calc_total_modularity2()<< endl;
			
		}
	}

	double calc_total_modularity() {
		double res = 0;
		for (auto& i : vnode) {
			int cnum = i.clustnum;
			for (auto& j : i.adj) {
				if (cnum != vnode[j.first].clustnum)continue;
				res += j.second - i.total_in*vnode[j.first].total_out/ total_weight;
			}

		}
		return res / total_weight;


	}
	double calc_total_modularity2() {
		double res = 0;
		for (auto& i : clustermap) {
			int cnum = i.first;
			double inweight = 0;
			for (auto& j : vnode) {
				if (j.clustnum == cnum) {
					for (auto& k : j.adj) {
						if (vnode[k.first].clustnum == cnum) {
							inweight += k.second;
						}
					}
				}
			}
			res += inweight - gamma*(i.second.total_in * i.second.total_out) / total_weight;


		}
		return res / total_weight;


	}
	graph generate_graph() {
		graph x;
		
		for (auto &i : clustermap) {
			int ct = x.vnode.size();
			node n(to_string(i.first), ct);
			
			x.vnode.push_back(n);
			x.nodemap[to_string(i.first)] = ct;
		}
		
		for (auto& i : vnode) {
			for (auto& j : i.adj) {
				if (vnode[j.first].clustnum != i.clustnum) {
					x.vnode[x.nodemap[to_string(i.clustnum)]].add_edge(x.nodemap[to_string(vnode[j.first].clustnum)], j.second);
					x.vnode[x.nodemap[to_string(vnode[j.first].clustnum)]].inlist.insert(x.nodemap[to_string(i.clustnum)]);
					x.vnode[x.nodemap[to_string(vnode[j.first].clustnum)]].total_in += j.second;
					x.total_weight += j.second;
				}

			}
		}
		x.init_clust();
		return x;
	}

	void print_cluster() {
		for (auto& i : clustermap) {
			cout << i.first << ":\n";
			for (int j = 0; j <vnode.size(); j++) {
				if (vnode[j].clustnum == i.first) {
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
			cout << "\n" << endl;
		}
	}
	void print_graph_detail() {
		cout << "ttl:" << total_weight << endl;
	}

};
