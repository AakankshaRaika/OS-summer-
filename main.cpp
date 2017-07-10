#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <stack>
#include <unordered_map>
#include <queue>

using namespace std;

class FrameContents{

public:
	int pageNumber;
	int pageAge;
	int pageFifoNum;
	FrameContents();
	~FrameContents();
};

vector<int> inFile(); //to get file
void onStart();       //start page
void initializeVector(vector<FrameContents>& frameVector);
int fifo(vector<int> pageVector, int frameNum);
int lru(vector<int> pageVector, int frameNum);
int lfu(vector<int> pageVector, int frameNum);
void incrementFrameAge(vector<FrameContents>& frameVector);
int lruStack(vector<int> pageVector, int frameNum);
bool checkAndReplace(stack<int>& frameStack, int frameNum, int page, int& replacemens);
void removeBottomPage(stack<int>& frameStack);
bool checkDuplicate(stack<int>& frameStack, int page);


int main(){
	onStart();
	return 0;
}

//constructor
FrameContents::FrameContents(){
	pageAge = 0;
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

	cout << "LRU protocol\n";
	cout << "Replacements: " << lru(holdPages, frameNum) << endl;

	cout << "LRU-STACK\n";
	cout << "Replacements: " << lruStack(holdPages, frameNum) << endl;
	
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

int lru(vector<int> pageVector, int frameNum){
	vector<FrameContents> frameVector(frameNum); //holds frame contents
	int countIncomingPages = 0;
	int frameCounter = 0;   //to get the index of the frame to replace
	FrameContents oldestPage; //to store oldest page
	int oldestIndex = 0;
	int replaceCount = 0;
	for (vector<int>::iterator it = pageVector.begin(); it != pageVector.end(); ++it){  //to loop through incoming pages
		bool hit = false;
		frameCounter = 0;
		oldestPage.pageAge = -1;
		for (vector<FrameContents>::iterator iter = frameVector.begin(); iter != frameVector.end(); ++iter){
			if (iter->pageNumber == *it){  //if the page has been hit
				cout << "hit\n";
				hit = true;
				iter->pageFifoNum = countIncomingPages;   //update the fifo number
				iter->pageAge = -1;
				break;
			}
			else{   //else find the oldest page and store it in a temp
				
					if(oldestPage.pageAge == iter->pageAge && oldestPage.pageFifoNum < iter->pageAge){	
						oldestIndex = frameCounter;
						oldestPage.pageAge = iter->pageAge;
						oldestPage.pageNumber = iter->pageNumber;
						oldestPage.pageFifoNum = iter->pageFifoNum;
					}
					else if (oldestPage.pageAge < iter->pageAge){
						oldestIndex = frameCounter;
						oldestPage.pageAge = iter->pageAge;
						oldestPage.pageNumber = iter->pageNumber;
						oldestPage.pageFifoNum = iter->pageFifoNum;
					}

				}
			
			frameCounter++;
		}
		if (hit == false){
			cout << "miss\n";
			replaceCount++;
			frameVector.at(oldestIndex).pageAge = -1;
			frameVector.at(oldestIndex).pageFifoNum = countIncomingPages;
			frameVector.at(oldestIndex).pageNumber = *it;
		}
		incrementFrameAge(frameVector);
		countIncomingPages++;
	}

	return replaceCount;
}

int lfu(vector<int> pageVector, int framNum){
	unordered_map<int,int> frameMap;


	return 0;
}

int lruStack(vector<int> pageVector, int frameNum){
	stack<int> frameStack;
	int replacementCount = 0;

	for (vector<int>::iterator it = pageVector.begin(); it != pageVector.end(); ++it){
		checkAndReplace(frameStack, frameNum, *it, replacementCount);
	}

	return replacementCount;
	
	
}

void initializeVector(vector<FrameContents>& frameVector){
	for (vector<FrameContents>::iterator it = frameVector.begin(); it != frameVector.end(); ++it){
		it->pageNumber = -1;
		it->pageAge = 0;
		it->pageFifoNum = -1;
	}
}

void incrementFrameAge(vector<FrameContents>& frameVector){
	for (vector<FrameContents>::iterator it = frameVector.begin(); it != frameVector.end(); ++it){
		it->pageAge++;
	}
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