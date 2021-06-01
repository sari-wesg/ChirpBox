import cbmng_common
import os

class myExpFirmwareApproach(cbmng_common.ExpFirmwareApproach):
	firmware = ""
	def read_configuration(self):
		if(cbmng_common.ExpFirmwareApproach.firmware_file.endswith(".bin") == True):
			with open(cbmng_common.ExpFirmwareApproach.firmware_file, 'rb') as f:
				firmware = f.read()	
			with open(os.path.join(os.path.dirname(__file__), "tmp_exp_firm.bin"), 'wb') as f:
	 			f.write(firmware)
		else:
			if os.path.exists(os.path.join(os.path.dirname(__file__), "tmp_exp_firm.bin")):
				os.remove(os.path.join(os.path.dirname(__file__), "tmp_exp_firm.bin"))
			print("Please prepare a .bin file")