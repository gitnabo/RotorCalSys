
#pragma once
#ifndef SerialPort_h__
#define SerialPort_h__

#pragma once


/**
@brief Simple serial port class.

This is a simple wrapper around a windows serial port.

No longer true, but don't forget this folklore in case it pops up:

SerialPort is derived from TWorkThread for the sole purpose
of shutting it down.  In some situations, the call to ::CloseHandle()
hangs. Close() defers the actual closing to the internal thread
so that it can timeout if it hangs. Other than that, CSerialPort is
NOT thread-aware.
*/

#include <QIODevice>

typedef void *HANDLE;
typedef struct _COMMTIMEOUTS COMMTIMEOUTS;
typedef struct _DCB DCB;

class SerialPort : public QIODevice
{
    Q_OBJECT
public:
    SerialPort(QObject *parent = nullptr);
    virtual ~SerialPort();

    int CommPort();
    void SetCommPort(int iPort);
    bool open(OpenMode mode) override;
    void close() override;


    void Open(int iPort, int iBaud);	///< Simple overload for common usage. Don't need DCB.

    virtual bool isSequential() { return true; }

    void SetDCB(DCB *pDCB);
    void GetDCB(DCB *pDCB);
    void SetTimeouts(COMMTIMEOUTS *pTimeouts);
    void GetTimeouts(COMMTIMEOUTS *pTimeouts);

    void GetCommMask(ulong *puiEvtMask);
    void SetCommMask(ulong uiEvtMask);

    qint64 writeData(const char *pData, qint64 qiSize) override;
    qint64 readData(char *pData, qint64 qiSize) override;
    void Flush(void);

    bool IsOpen() { return m_hFile != 0; }
    HANDLE GetHandle(void) { return m_hFile; }

    static QList<int> ListCommPorts();

private:
    HANDLE m_hFile;
    uint m_iCommPort;
};
#endif // SerialPort_h__

