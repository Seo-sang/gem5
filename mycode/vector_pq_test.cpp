#include <stdio.h>
#include <stdint.h>
#include <queue>
#include <stdlib.h>

#include "./tokenize/vector_pq.h"

struct node {
	int32_t score;
	int32_t pos;
	void* ptr;

	struct compare {
		bool operator() (const struct node &a, const struct node &b) {
			return (a.score < b.score) || (a.score == b.score && a.pos > b.pos);
		}
	};

	node() {}
	node(int32_t score, int32_t pos, void* ptr) : score(score), pos(pos), ptr(ptr) {}

};


int64_t arr[22] = {0,
	(15LL << 32) + 1, (14LL << 32) + 2, (13LL << 32) + 3, (12LL << 32) + 1,
	(11LL << 32) + 1, (10LL << 32) + 1, (13LL << 32) + 5, (8LL << 32) + 1, (7LL << 32) + 1,
	(6LL << 32) + 1, (5LL << 32) + 1, (4LL << 32) + 1, (3LL << 32) + 1, (2LL << 32) + 1, (1LL << 32) + 6,
	(74LL << 32) + 4, (7LL << 32) + 1, (11LL << 32) + 1, (14LL << 32) + 1, (4LL << 32) + 1, (2LL << 32) + 6,
};

int main() {

	class my_pq my_queue(5000);
	std::priority_queue<struct node, std::vector<struct node>, struct node::compare> queue;


	int total_cnt = 0;
	int correct_cnt = 0;
	int cnt = 0;

	for(int i = 1; i < 44; i++) {

		if(i < 22) {
		//if(cnt == 0 || behavior) {
			printf("--------push---------\n");
			printf("score: %ld, pos: %ld\n", arr[i] >> 32, arr[i] & 0xffffffff);
			printf("---------------------\n");
			queue.push(node(arr[i] >> 32, arr[i] & 0xffffffff, nullptr));
            struct llm_bigram_spm new_node;
            new_node.score = arr[i] >> 32;
            new_node.left = arr[i] & 0xffffffff;
			my_queue.push(new_node);
			cnt++;
		}
		else {
			if(!my_queue.empty()) {
				struct node answer = queue.top();
				struct llm_bigram_spm my_answer = my_queue.top();

				if(my_answer.score != answer.score && my_answer.left != answer.pos) {
					printf("---------pop---------\n");
					printf("my answer : %d, %d\n", my_answer.score, my_answer.left);
					printf("answer : %d, %d\n", answer.score, answer.pos);
					printf("---------------------\n");
				}

				if(my_answer.score == answer.score && my_answer.left == answer.pos) correct_cnt++;
				total_cnt++;

				queue.pop();
				my_queue.pop();
			}
		}

	}



	printf("Total input: %d\n", total_cnt);
	printf("Correct answers: %d\n", correct_cnt);


}
