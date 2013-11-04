import collections
import logging
import subprocess
import types
import cli, storage, surl 


def _getVoName():
    cmdArray = ['voms-proxy-info', '--all']
    proc = subprocess.Popen(cmdArray, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
    rcode = proc.wait()
    if rcode != 0:
        raise Exception("Could not get the VO name!")
    for line in proc.stdout.readlines():
        if ':' in line:
            (key, value) = line.split(':', 1)
            key = key.strip()
            value = value.strip()
            if key == 'VO':
                return value


class TestBase():

    def __init__(self, *args, **kwargs): 
        self.surl   = surl.Surl()
        self.client = cli.Cli()
        self.voName = _getVoName()


    def setUp(self):
        self.transfers = []
        for (srcSa, dstSa) in storage.getStoragePairs():
            srcSurl = self.surl.generate(srcSa)
            dstSurl = self.surl.generate(dstSa)
            self.transfers.append((srcSurl, dstSurl))


    def _flatten(self, l):
        if type(l) is types.StringType:
            yield l
        else:
            for i in l:
                if isinstance(i, collections.Iterable):
                    for sub in self._flatten(i):
                        yield sub
                else:
                    yield i


    def _removeFiles(self, list):
        allFiles = self._flatten(list)
        for f in allFiles:
            try:
                logging.debug("Removing %s" % f)
                self.surl.unlink(f)
            except Exception, e:
                logging.warning(e)


    def tearDown(self):
        self._removeFiles(self.transfers)
        

    def run(self):
        testMethods = [getattr(self, method) for method in dir(self) if method.startswith('test_') and callable(getattr(self, method))]
        nErrors = 0
        for test in testMethods:
            try:
                self.setUp()
                logging.info("Running %s" % test.__name__)
                for line in test.__doc__.split('\n'):
                    logging.info(line)
                test()
            except Exception, e:
                logging.error(str(e))
                nErrors += 1
            finally:
                self.tearDown()
        logging.info("%d test executed, %d failed" % (len(testMethods), nErrors))
        return nErrors


    def assertEqual(self, expected, value, label = None):
        if expected != value:
            if label:
                raise AssertionError("Expected '%s' == '%s', got '%s'" % (label, str(expected), str(value)))
            else:
                raise AssertionError("Expected '%s', got '%s'" % (str(expected), str(value)))
        if label:
            logging.info("Assertion '%s' == '%s' OK" % (label, str(value)))
        else:
            logging.info("Assertion '%s' == '%s' OK" % (str(expected), str(value)))


    def assertRaises(self, excType, method, *args, **kwargs):
        try:
            method(*args, **kwargs)
            raise AssestionError("Expected exception '%s', none raised" % (excType.__name__))
        except Exception, e:
            if not isinstance(e, excType):
                raise AssertionError("Expected exception '%s', got '%s'" % (excType.__name__, type(e).__name__))
            logging.info("Exception '%s' raised as expected" % excType.__name__)


    def assertLessEqualThan(self, value, limit, label = None):
        if value > limit:
            if label:
                raise AssertionError("Expected %s <= %s, but %s is higher" % (label, str(limit), str(value)))
            else:
                raise AssertionError("Expected <= %s, but %s is higher" % (str(limit), str(value)))
        if label:
            logging.info("Assertion %s <= %s OK" % (label, str(limit)))
        else:
            logging.info("Assertion %s <= %s OK" % (str(value), str(limit)))


    def assertIn(self, expected, value, label = None):
        if value not in expected:
            if label:
                raise AssertionError("Expected '%s' in [%s], got %s" % (label, ','.join(expected), str(value)))
            else:
                raise AssertionError("Expected a value in [%s], got %s" % (','.join(expected), str(value)))
        if label:
            logging.info("Assertion '%s' in [%s] OK (%s)" % (label, ','.join(expected), str(value)))
        else:
            logging.info("Assertion '%s' in [%s] OK" % (str(value), ','.join(expected)))

