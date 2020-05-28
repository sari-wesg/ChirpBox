import cbmng_exp_config
import cbmng_common
import datetime
import time
import json

exp_conf = "tmp_exp_conf.json"
firmware = "tmp_exp_firm.bin"
exp_meth = "tmp_exp_meth.json"
running_status = "tmp_exp_running.json"

expconfapp = cbmng_exp_config.myExpConfApproach()

def check():
	try:
		f = open(exp_conf, mode = 'r')
		f.close()
	except FileNotFoundError:
		print("Please complete experiment completion with \"-ec\"")
		return False;
	except PermissionError:
		print("Please complete experiment completion with \"-ec\"")
		return False;

	try:
		f = open(firmware, mode = 'r')
		f.close()
	except FileNotFoundError:
		print("Please complete experiment firmware with \"-ef\"")
		return False;
	except PermissionError:
		print("Please complete experiment firmware with \"-ef\"")
		return False;

	try:
		f = open(exp_meth, mode = 'r')
		f.close()
	except FileNotFoundError:
		print("Please complete experiment methodology with \"-em\"")
		return False;
	except PermissionError:
		print("Please complete experiment methodology with \"-em\"")
		return False;

	return True

def start():
	if(expconfapp.experiment_configuration(exp_conf) == True):
		expconfapp.read_configuration()
		time_now = datetime.datetime.now()
		start_time_t = time_now + datetime.timedelta(minutes = 2)
		start_time = start_time_t.strftime("%Y-%m-%d %H:%M:%S")
		end_time_t = start_time_t + datetime.timedelta(minutes = expconfapp.experiment_duration)
		end_time = end_time_t.strftime("%Y-%m-%d %H:%M:%S")
		exp_no = cbmng_common.tid_maker()
		print("Experiment #" + exp_no + " is going to start at " + start_time + ", and stop at " + end_time)
		running_dict = {'exp_number': exp_no, 'start_time': time_now.strftime("%Y-%m-%d %H:%M:%S"), 'end_time': end_time_t.strftime("%Y-%m-%d %H:%M:%S"), 'duration': expconfapp.experiment_duration}
		with open(running_status, "w") as f:
			json.dump(running_dict, f)
		# TODO: Add the serial command to start
		return True
	else:
		return False

def is_running():
	try:
		f = open(running_status, mode = 'r')
		f.close()
	except FileNotFoundError:
		return False;
	except PermissionError:
		return False;

	with open(running_status, 'r') as load_f:
		running_dict = json.load(load_f)
		start_time_t = datetime.datetime.strptime(running_dict['start_time'], '%Y-%m-%d %H:%M:%S')
		end_time_t = datetime.datetime.strptime(running_dict['end_time'], '%Y-%m-%d %H:%M:%S')
		time_now = datetime.datetime.now()
		if (time_now >= start_time_t) and (time_now <= end_time_t):
			return True
		else:
			return False

def connectivity_evaluation(sf, channel):
	time_now = datetime.datetime.now()
	start_time_t = time_now + datetime.timedelta(minutes = 2)
	start_time = start_time_t.strftime("%Y-%m-%d %H:%M:%S")
	end_time_t = start_time_t + datetime.timedelta(minutes = 10)
	end_time = end_time_t.strftime("%Y-%m-%d %H:%M:%S")
	exp_no = cbmng_common.tid_maker()
	print("Connectivity evaluation (SF " + str(sf) + ", Channel " + str(channel) + " MHz) is going to start at " + start_time + ", and stop at " + end_time)
	running_dict = {'connectivity_evaluation': exp_no, 'start_time': time_now.strftime("%Y-%m-%d %H:%M:%S"), 'end_time': end_time_t.strftime("%Y-%m-%d %H:%M:%S"), 'duration': 10}
	with open(running_status, "w") as f:
		json.dump(running_dict, f)
	# TODO: Add the serial command to start	
	return True