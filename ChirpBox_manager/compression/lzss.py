import ctypes
import os

class LZSS():
    def __init__(self, preBufSizeBits):
        self.threshold = 2  #长度大于等于2的匹配串才有必要压缩
        self.preBufSizeBits = preBufSizeBits  #前向缓冲区占用的比特位
        self.windowBufSizeBits = 16 - self.preBufSizeBits   #滑动窗口占用的比特位

        self.preBufSize = (1 << self.preBufSizeBits) - 1 + self.threshold #通过占用的比特位计算缓冲区大小
        self.windowBufSize = (1 << self.windowBufSizeBits) - 1 + self.threshold   #通过占用的比特位计算滑动窗口大小

        self.preBuf = b''   #前向缓冲区
        self.windowBuf = b''    #滑动窗口
        self.matchString = b''  #匹配串
        self.matchIndex = 0     #滑动窗口匹配串起始下标

    #文件压缩
    def LZSS_encode(self, readfilename, writefilename):

        fread = open(readfilename, "rb")
        fwrite = open(writefilename, "wb")
        restorebuff = b''   #待写入的数据缓存区，满一组数据写入一次文件
        itemnum = 0     #8个项目为一组，用来统计当前项目数
        signbits = 0    #标记字节

        self.preBuf = fread.read(self.preBufSize)   #读取数据填满前向缓冲区

        # 前向缓冲区没数据可操作了即为压缩结束
        while self.preBuf != b'':
            self.matchString = b''
            self.matchIndex = -1
            #在滑动窗口中寻找最长的匹配串
            for i in range(self.threshold, len(self.preBuf) + 1):
                index = self.windowBuf.find(self.preBuf[0:i])
                if index != -1:
                    self.matchString = self.preBuf[0:i]
                    self.matchIndex = index
                else:
                    break
            #如果没找到匹配串或者匹配长度为1，直接输出原始数据
            if self.matchIndex == -1:
                self.matchString = self.preBuf[0:1]
                restorebuff += self.matchString
            else:
                restorebuff += bytes(ctypes.c_uint16(self.matchIndex * (1 << self.preBufSizeBits) + len(self.matchString) - self.threshold))
                signbits += (1 << (7 - itemnum))
            #操作完一个项目+1
            itemnum += 1
            #项目数达到8了，说明做完了一组压缩，将这一组数据写入文件
            if itemnum >= 8:
                writebytes = bytes(ctypes.c_uint8(signbits)) + restorebuff
                fwrite.write(writebytes);
                itemnum = 0
                signbits = 0
                restorebuff = b''

            self.preBuf = self.preBuf[len(self.matchString):]  #将刚刚匹配过的数据移出前向缓冲区
            self.windowBuf += self.matchString  #将刚刚匹配过的数据加入滑动窗口
            if len(self.windowBuf) > self.windowBufSize:  #将多出的数据从前面开始移出滑动窗口
                self.windowBuf = self.windowBuf[(len(self.windowBuf) - self.windowBufSize):]

            self.preBuf += fread.read(self.preBufSize - len(self.preBuf))  #读取数据补充前向缓冲区

        if restorebuff != b'':  #文件最后可能不满一组数据量，直接写到文件里
            writebytes = bytes(ctypes.c_uint8(signbits)) + restorebuff
            fwrite.write(writebytes);

        fread.close()
        fwrite.close()

        return os.path.getsize(writefilename)

    #文件解压
    def LZSS_decode(self, readfilename, writefilename):
        fread = open(readfilename, "rb")
        fwrite = open(writefilename, "wb")

        self.windowBuf = b''
        self.preBuf = fread.read(1)  #先读一个标记字节以确定接下来怎么解压数据

        while self.preBuf != b'':
            for i in range(8):  #8个项目为一组进行解压
                # 从标记字节的最高位开始解析，0代表原始数据，1代表(下标，匹配数)解析
                if self.preBuf[0] & (1 << (7 - i)) == 0:
                    temp = fread.read(1)
                    fwrite.write(temp)
                    self.windowBuf += temp
                else:
                    temp = fread.read(2)
                    start = ((temp[0] + temp[1] * 256) // (1 << self.preBufSizeBits))  #取出高位的滑动窗口匹配串下标
                    end = start + temp[0] % (1 << self.preBufSizeBits) + self.threshold  #取出低位的匹配长度
                    fwrite.write(self.windowBuf[start:end])  #将解压出的数据写入文件
                    self.windowBuf += self.windowBuf[start:end]  #将解压处的数据同步写入到滑动窗口

                if len(self.windowBuf) > self.windowBufSize:  #限制滑动窗口大小
                    self.windowBuf = self.windowBuf[(len(self.windowBuf) - self.windowBufSize):]

            self.preBuf = fread.read(1)  #读取下一组数据的标志字节

        fread.close()
        fwrite.close()


# if __name__ == '__main__':
#     Demo = LZSS(7)
#     Demo.LZSS_encode("115.bin", "encode.bin")
#     Demo.LZSS_decode("encode.bin", "decode.bin")



