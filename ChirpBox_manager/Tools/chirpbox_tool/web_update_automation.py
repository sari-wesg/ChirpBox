# Original Author: 130L
# free to use, modify and share, but don't get rid of this header
# TODO: need to change shell=False to shell=True in subprocess.py __init__
import subprocess
import logging

TIMEOUT = 90
YOUR_HEXO_DIR = 'D:\\TP\\Study\\Hexo-Chirpbox\\blog'

def gd(logger):
	'''
	this function is for hexo g command
	'''
	logger.debug('\ngenerating......\n')
	proc = subprocess.Popen(['hexo', 'g'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True, cwd=YOUR_HEXO_DIR)
	fail = False

	try:
		outs, errs = proc.communicate(timeout=TIMEOUT)
		if outs:
			logger.debug('\n' + outs.decode('utf-8'))
		if errs:
			logger.error('\n' + errs.decode('gbk'))
			fail = True
	except subprocess.TimeoutExpired:
		proc.kill()
		outs, errs = proc.communicate()
		if outs:
			logger.debug('\n'+outs.decode('utf-8'))
		if errs:
			logger.error('\n'+errs.decode('utf-8'))
		fail = True
	logger.debug('\nfinish generating, result {}\n'.format(('FAILED' if fail else 'SUCCESS')))
	deploy(logger)

def deploy(logger):
	'''
	this function is for hexo d command
	'''
	proc = subprocess.Popen(['hexo', 'd'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True, cwd=YOUR_HEXO_DIR)
	logger.debug('\ndeploying......\n')
	fail = False
	try:
		outs, errs = proc.communicate(timeout=TIMEOUT)
		if outs:
			logger.debug('\n'+outs.decode('utf-8'))
		if errs:
			logger.error('\n'+errs.decode('gbk'))
			fail = True
	except subprocess.TimeoutExpired:
		proc.kill()
		outs, errs = proc.communicate()
		if outs:
			logger.debug('\n'+outs.decode('utf-8'))
		if errs:
			logger.error('\n'+errs.decode('utf-8'))
		fail = True
	logger.debug('\nfinish deployer, result {}\n'.format(('FAILED' if fail else 'SUCCESS')))

# create a logger

logger = logging.getLogger(__name__)
handler = logging.StreamHandler()
formatter = logging.Formatter(
        '%(asctime)s %(levelname)-8s %(message)s')
handler.setFormatter(formatter)
logger.addHandler(handler)
logger.setLevel(logging.DEBUG)

# hexo g -d

gd(logger)
