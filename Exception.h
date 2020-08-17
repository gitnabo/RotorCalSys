#pragma once
#include <QString>


/**
@brief Exception base class

*/
class Exception : public QString {
public:
	Exception();
	Exception(QString sMsg);
};



class AbortException : public Exception {
public:
	AbortException() {}
};