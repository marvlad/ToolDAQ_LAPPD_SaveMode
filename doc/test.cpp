#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <algorithm>
#include <fstream>		

using namespace std;

int main()
{
	vector<int> boardsReadyForRead = {0,1,5,6};

	sort(boardsReadyForRead.begin(), boardsReadyForRead.end());

	bool control = false;
	if(boardsReadyForRead.size()%2==0)
	{
		for(int m=0; m<boardsReadyForRead.size(); m+=2)
		{
			if(boardsReadyForRead[m+1]-boardsReadyForRead[m]==1)
			{
				control = true;
			}else 
			{
				control = false;
			}
		}
		if(control==true)
		{
			cout << "YAY" << endl;
		}else
		{
			cout << "NAY" << endl;
		}
	}else
	{
		cout << "NAY 2" << endl;
	}

	return 1;
}