#pragma once

#include <stdint.h>
#include <string>

class Transfer
{
public:
    unsigned    fileId;
    std::string jobId;
    std::string sourceUrl;
    std::string destUrl;
    std::string checksumAlgorithm;
    std::string checksumValue;
    double      userFileSize;
    std::string fileMetadata;
    std::string tokenBringOnline;

    // Timestamps in milliseconds
    uint64_t startTime;
    uint64_t finishTime;

    // File size
    off_t fileSize;

    // Progress makers
    double throughput;
    off_t transferredBytes;

    Transfer(): fileId(0), userFileSize(0), startTime(0), finishTime(0),
        fileSize(0), throughput(0.0), transferredBytes(0)
    {
    }

    void setChecksum(const std::string& checksum)
    {
        size_t colon = checksum.find(':');
        if (colon == std::string::npos)
            {
                checksumAlgorithm.assign(checksum);
                checksumValue.clear();
            }
        else
            {
                checksumAlgorithm.assign(checksum.substr(0, colon));
                checksumValue.assign(checksum.substr(colon + 1));
            }
    }

    double getTransferDurationInSeconds()
    {
        if (startTime == 0 || finishTime == 0)
            return 0.0;
        return (static_cast<double>(finishTime) - static_cast<double>(startTime)) / 1000.0;
    }

    // Create a transfer out of a string
    static Transfer createFromString(const std::string& jobId, const std::string& line)
    {
        typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

        std::string strArray[7];
        tokenizer tokens(line, boost::char_separator<char> (" "));
        std::copy(tokens.begin(), tokens.end(), strArray);

        Transfer t;
        t.jobId = jobId;
        t.fileId = boost::lexical_cast<unsigned>(strArray[0]);
        t.sourceUrl = strArray[1];
        t.destUrl   = strArray[2];
        t.setChecksum(strArray[3]);
        t.userFileSize = boost::lexical_cast<double>(strArray[4]);
        t.fileMetadata = strArray[5];
        t.tokenBringOnline = strArray[6];
        return t;
    }

    // Create a transfer out of the command line options
    static Transfer createFromOptions(const UrlCopyOpts& opts)
    {
        Transfer t;
        t.jobId = opts.jobId;
        t.fileId = opts.fileId;
        t.sourceUrl = opts.sourceUrl;
        t.destUrl   = opts.destUrl;
        t.setChecksum(opts.checksumValue);
        t.userFileSize = opts.userFileSize;
        t.fileMetadata = opts.fileMetadata;
        t.tokenBringOnline = opts.tokenBringOnline;
        return t;
    }

    // Initialize a list from a file
    static void initListFromFile(const std::string& jobId, const std::string& path,
                                 std::vector<Transfer>* list)
    {
        std::string line;
        std::ifstream infile(path.c_str(), std::ios_base::in);

        while (getline(infile, line, '\n'))
            {
                Transfer t = Transfer::createFromString(jobId, line);
                list->push_back(t);
            }

        infile.close();
        unlink(path.c_str());
    }
};
