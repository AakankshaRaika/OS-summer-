#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <stack>
#include <unordered_map>
#include <queue>


using namespace std;



unordered_map<int, int> pageMap;
vector<int> inFile(); //to get file
void onStart();       //start page
int fifo(vector<int> pageVector, int frameNum);
int lfu(vector<int> pageVector, int frameNum);
int lruStack(vector<int> pageVector, int frameNum);
bool checkAndReplace(stack<int>& frameStack, int frameNum, int page, int& replacemens);
bool checkDuplicate(stack<int>& frameStack, int page);

class FrameContents{

public:
	int pageNumber;
	int age;
	FrameContents();
	~FrameContents();

	friend bool operator<(const FrameContents& lhs, const FrameContents& rhs)
	{
		
		if (pageMap.at(lhs.pageNumber) < pageMap.at(rhs.pageNumber)){
			return true;
		}
		else{
			if (pageMap.at(lhs.pageNumber) == pageMap.at(rhs.pageNumber)){
				return(lhs.age < rhs.age);
			}
			else{
				return false;
			}
		}
	}
};



int main(){
	onStart();
	return 0;
}

//constructor
FrameContents::FrameContents(){
	pageNumber = 0;
}

//deconstructor
FrameContents::~FrameContents(){

}

void onStart(){
	int frameNum = 0;
	vector<int> holdPages;

	cout << "Enter the frames you wish to use\n"; //error check?
	cin >> frameNum;
	holdPages = inFile();  //getting pages 

	cout << "FIFO protocol: \n";
	cout << "Replacements: " << fifo(holdPages, frameNum) << endl;

	cout << "LRU-STACK\n";
	cout << "Replacements: " << lruStack(holdPages, frameNum) << endl;

	cout << "LFU\n";
	cout << "Replacements: " << lfu(holdPages, frameNum) << endl;

}

vector<int> inFile(){
	string line;
	string filePath;
	vector<int> holdFrames;

	cout << "Please enter the full file path\n"; //error check?
	cin >> filePath;
	ifstream myfile(filePath);
	if (myfile.is_open())   //opening file path to put frames in vector
	{
		int getNum = 0;
		while (myfile >> getNum)
		{
			holdFrames.push_back(getNum);
			//cout << getNum << '\n';
		}
		myfile.close();
	}

	else cout << "Unable to open file\n";
	return holdFrames;
}

int fifo(vector<int> pageVector, int frameNum){
	queue<int> frameQueue;
	queue<int> tempQueue;
	int replaceCount = 0;
	bool duplicate = false;

	for (vector<int>::iterator it = pageVector.begin(); it != pageVector.end(); ++it){
		if (frameQueue.size() < frameNum){
			frameQueue.push(*it);
			//cout << "miss\n";
			++replaceCount;
		}
		else{
			while (!frameQueue.empty()){
				tempQueue.push(frameQueue.front());
				if (frameQueue.front() == *it){
					duplicate = true;
					//cout << "hit\n";
				}
				frameQueue.pop();
			}
			if (duplicate == true){
				while (!tempQueue.empty()){
					frameQueue.push(tempQueue.front());
					tempQueue.pop();
				}
				duplicate = false;
			}
			else{
				//cout << "Miss\n";
				++replaceCount;
				while (!tempQueue.empty()){
					//cout << "In queue: " << tempQueue.front();
					frameQueue.push(tempQueue.front());
					tempQueue.pop();
				}
				frameQueue.pop();
				frameQueue.push(*it);
			}
		}
	}

	return replaceCount;
}

int lfu(vector<int> pageVector, int frameNum){
	priority_queue<FrameContents> priorQueueFrame;
	FrameContents tempFrame;
	FrameContents deletePage;
	int countPages = 0;
	

	for (vector<int>::iterator it = pageVector.begin(); it != pageVector.end(); ++it){
		if (pageMap.find(*it) == pageMap.end()){
			tempFrame.age = *it;
			tempFrame.pageNumber = countPages;
			pageMap.insert({ *it, 0 });
			if (priorQueueFrame.size() < frameNum){
				priorQueueFrame.push(tempFrame);
			}
			else{
				deletePage = priorQueueFrame.top();
				pageMap.erase(deletePage.pageNumber);
				priorQueueFrame.push(tempFrame);
				
			}
			cout << "miss\n";
		}
		else{
			cout << "hit\n";
			pageMap.at(*it) = pageMap.at(*it) + 1;
		}
		countPages++;
	}
	return 0;
}

int lruStack(vector<int> pageVector, int frameNum){
	stack<int> frameStack;
	int replacements = 0;

	for (vector<int>::iterator it = pageVector.begin(); it != pageVector.end(); ++it){
		checkAndReplace(frameStack, frameNum, *it, replacements);
	}

	return replacements;
}
//checks for page in stack, if found, puts at top, if not, takes last out and puts replacement at top
bool checkAndReplace(stack<int>& frameStack, int frameNum, int page, int& replacements){
	stack<int> tempStack;
	int pageCheck;
	bool pageFound = false;

	if (frameStack.size() < frameNum){
		cout << "miss\n";
		++replacements;
		if (checkDuplicate(frameStack, page)){

		}
		else{
			frameStack.push(page);
		}
	}
	else{
		if (checkDuplicate(frameStack, page)){
			cout << "hit\n";
		}
		else{
			++replacements;
			cout << "miss\n";
			while (!frameStack.empty()){
				tempStack.push(frameStack.top());
				frameStack.pop();
			}
			tempStack.pop();
			while (!tempStack.empty()){
				frameStack.push(tempStack.top());
				tempStack.pop();
			}
			frameStack.push(page);
		}
	}
	return true;
}

bool checkDuplicate(stack<int>& frameStack, int page){
	stack<int> tempStack;
	int checkPage;
	bool duplicate = false;

	while (!frameStack.empty()){
		checkPage = frameStack.top();
		if (checkPage == page){
			frameStack.pop();
			duplicate = true;
		}
		if (!frameStack.empty()){
			tempStack.push(frameStack.top());
			frameStack.pop();
		}
	}

	while (!tempStack.empty()){
		frameStack.push(tempStack.top());
		tempStack.pop();
	}
	if (duplicate)
		frameStack.push(page);
	return duplicate;

}
