import datetime
import os

class ExpConfApproach:
	conf_json = ''
	def experiment_configuration(self, conf_json):
		try:
			f = open(conf_json, mode = 'r')
			f.close()
			print("The file " + conf_json + " has been read as experiment configuration of Chirpbox.")
			ExpConfApproach.conf_json = conf_json
			return True
		except FileNotFoundError:
			print("File is not found.")
			return False
		except PermissionError:
			print("You don't have permission to access this file.")
			return False

class ExpFirmwareApproach:
	firmware_file = ''
	def experiment_firmware(self, firmware_file):
		try:
			f = open(firmware_file, mode = 'r')
			f.close()
			print("The file " + firmware_file + " has been read as experiment firmware file of Chirpbox.")
			ExpFirmwareApproach.firmware_file = firmware_file
			return True
		except FileNotFoundError:
			print("File is not found.")
			return False
		except PermissionError:
			print("You don't have permission to access this file.")
			return False

class ExpMethodApproach:
	methodology_file = ''
	def experiment_methodology(self, methodology_file):
		try:
			f = open(methodology_file, mode = 'r')
			f.close()
			print("The file " + methodology_file + " has been read as methodology file of Chirpbox.")
			ExpMethodApproach.methodology_file = methodology_file
			return True
		except FileNotFoundError:
			print("File is not found.")
			return False
		except PermissionError:
			print("You don't have permission to access this file.")
			return False

def tid_maker():
	return '{0:%Y%m%d%H%M%S%f}'.format(datetime.datetime.now())

def get_FileSize(filePath):
	#filePath = unicode(filePath,'utf8')
	fsize = os.path.getsize(filePath)
	#fsize = fsize/float(1024*1024)
	#return round(fsize,2)
	return fsize