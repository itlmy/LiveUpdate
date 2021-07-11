#pragma once
class CAntiMultiOpen
{
public:
	CAntiMultiOpen();
	~CAntiMultiOpen();

	bool checkMultiOpen();
	bool checkVM();
	void minusProcessCount();

private:
	bool checkMutex();
	bool checkDataSeg();

	bool checkVPC();
	bool checkVMWare();
	
};

