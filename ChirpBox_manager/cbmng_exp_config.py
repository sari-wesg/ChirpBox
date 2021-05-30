import cbmng_common
import json

class myExpConfApproach(cbmng_common.ExpConfApproach):
	payload_length = 0
	experiment_duration = 0
	experiment_name = ""
	experiment_description = ""

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

		with open("tmp_exp_conf.json","w") as dump_f:
			json.dump(load_dict,dump_f)
