import cbmng_common
import json

class myExpConfApproach(cbmng_common.ExpConfApproach):
	payload_length = 0
	experiment_duration = 0
	experiment_name = ""
	experiment_description = ""
	command_sf = 7

	def read_configuration(self):
		with open(cbmng_common.ExpConfApproach.conf_json,'r') as load_f:
			load_dict = json.load(load_f)
			# Experiment name:
			myExpConfApproach.experiment_name = load_dict['experiment_name']
			print("experiment_name: " + myExpConfApproach.experiment_name)
			# Experiment description:
			myExpConfApproach.experiment_description = load_dict['experiment_description']
			print("experiment_description: " + myExpConfApproach.experiment_description)
			# Payload length:
			myExpConfApproach.payload_length = load_dict['payload_length']
			print("payload_length: " + str(myExpConfApproach.payload_length) + " Byte(s)")
			# Experiment duration:
			myExpConfApproach.experiment_duration = load_dict['experiment_duration']
			print("experiment_duration: " + str(myExpConfApproach.experiment_duration) + " Second(s)")
			# Command SF:
			myExpConfApproach.command_sf = load_dict['command_sf']
			print("command_sf: " + str(myExpConfApproach.command_sf))

		with open("tmp_exp_conf.json","w") as dump_f:
			json.dump(load_dict,dump_f)
		# TODO: Add some codes here

class myExpPrimitiveConf(cbmng_common.ExpPrimitiveConf):
	primitive_name = ""
	com_port = ""
	primitive_config = []
	def read_configuration(self):
		with open(cbmng_common.ExpPrimitiveConf.exp_primitive_conf,'r') as load_f:
			load_dict = json.load(load_f)
			# For all primitives:
			# Experiment com port:
			myExpPrimitiveConf.com_port = load_dict['com_port']
			myExpPrimitiveConf.primitive_config.append(myExpPrimitiveConf.com_port)
		return (myExpPrimitiveConf.primitive_config)