import cbmng_common
import os

class myExpFirmwareApproach(cbmng_common.ExpFirmwareApproach):
	firmware = ""
	def read_configuration(self):
		if(cbmng_common.ExpFirmwareApproach.firmware_file.endswith(".bin") == True):
			with open(cbmng_common.ExpFirmwareApproach.firmware_file, 'rb') as f:
				firmware = f.read()	
			# TODO: patch firmware according to the experiment configuration
			with open('tmp_exp_firm.bin', 'wb') as f:
	 			f.write(firmware)
		else:
			if os.path.exists("tmp_exp_firm.bin"):
				os.remove("tmp_exp_firm.bin")
			print("Please prepare a .bin file")