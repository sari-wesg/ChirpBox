import cbmng_exp_config
import cbmng_exp_firm
import cbmng_exp_method
import cbmng_common
import datetime
import time
import json
import transfer_to_initiator.myserial.serial_send

import serial

exp_conf = "tmp_exp_conf.json"
firmware = "tmp_exp_firm.bin"
exp_meth = "tmp_exp_meth.json"
running_status = "tmp_exp_running.json"

expconfapp = cbmng_exp_config.myExpConfApproach()
expfirmapp = cbmng_exp_firm.myExpFirmwareApproach()
expmethapp = cbmng_exp_method.myExpMethodApproach()

def check():
	try:
		f = open(exp_conf, mode = 'r')
		f.close()
	except FileNotFoundError:
		print("Please complete experiment completion with \"-ec\"")
		return False
	except PermissionError:
		print("Please complete experiment completion with \"-ec\"")
		return False

	try:
		f = open(firmware, mode = 'r')
		f.close()
	except FileNotFoundError:
		print("Please complete experiment firmware with \"-ef\"")
		return False
	except PermissionError:
		print("Please complete experiment firmware with \"-ef\"")
		return False

	try:
		f = open(exp_meth, mode = 'r')
		f.close()
	except FileNotFoundError:
		print("Please complete experiment methodology with \"-em\"")
		return False
	except PermissionError:
		print("Please complete experiment methodology with \"-em\"")
		return False

	return True

def check_finished():
	try:
		f = open(running_status, mode = 'r')
		f.close()
	except FileNotFoundError:
		return False
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
		exp_name = expconfapp.experiment_name
		print("Experiment #" + exp_no + " (" + exp_name + ") is going to start at " + start_time + ", and stop at " + end_time)
		running_dict = {'exp_name': exp_name, 'exp_number': exp_no, 'start_time': time_now.strftime("%Y-%m-%d %H:%M:%S"), 'end_time': end_time_t.strftime("%Y-%m-%d %H:%M:%S"), 'duration': expconfapp.experiment_duration}
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
		return False

	with open(running_status, 'r') as load_f:
		running_dict = json.load(load_f)
		start_time_t = datetime.datetime.strptime(running_dict['start_time'], '%Y-%m-%d %H:%M:%S')
		end_time_t = datetime.datetime.strptime(running_dict['end_time'], '%Y-%m-%d %H:%M:%S')
		time_now = datetime.datetime.now()
		if (time_now >= start_time_t) and (time_now <= end_time_t):
			return True
		else:
			return False

def connectivity_evaluation(sf, channel, tx_power):
	time_now = datetime.datetime.now()
	start_time_t = time_now + datetime.timedelta(minutes = 2)
	start_time = start_time_t.strftime("%Y-%m-%d %H:%M:%S")
	end_time_t = start_time_t + datetime.timedelta(minutes = 10)
	end_time = end_time_t.strftime("%Y-%m-%d %H:%M:%S")
	exp_no = cbmng_common.tid_maker()
	exp_name = "Chirpbox_connectivity_sf" + str(sf) + "ch" + str(channel) + "tp" + str(tx_power)
	print("Connectivity evaluation (SF " + str(sf) + ", Channel " + str(channel) + " MHz, " + str(tx_power) + " dBm) is going to start at " + start_time + ", and stop at " + end_time)
	running_dict = {'exp_name': exp_name, 'exp_number': exp_no, 'start_time': time_now.strftime("%Y-%m-%d %H:%M:%S"), 'end_time': end_time_t.strftime("%Y-%m-%d %H:%M:%S"), 'duration': 10}
	with open(running_status, "w") as f:
		json.dump(running_dict, f)
	# TODO: Add the serial command to start	
	return True

def assign_sniffer():
	if(expmethapp.experiment_methodology(exp_meth) == True):
		expmethapp.read_configuration()
		time_now = datetime.datetime.now()
		print("The sniffer(s): " + str(expmethapp.sniffer))
		# TODO: Add the serial command to start
		return True
	else:
		return False

def collect_data():
	with open(running_status,'r') as load_f:
		load_dict = json.load(load_f)
		filename = load_dict['exp_name'] +"(" + load_dict['exp_number'] + ")"
	print("Collecting ...")
	# TODO: Serial commands here...
	print("Results of " + filename + " have been collected!" )
	return True

def collect_topology():
	with open(running_status,'r') as load_f:
		load_dict = json.load(load_f)
		filename = load_dict['exp_name'] +"(" + load_dict['exp_number'] + ")"
	print("Collecting ...")
	# TODO: Serial commands here...
	print("Results of " + filename + " have been collected!" )
	return True	

def disseminate():
	# config and open the serial port
	ser = transfer_to_initiator.myserial.serial_send.config_port('COM10')
	# corresponded task number
	disseminate_task = 2

	timeout_cnt = 0

	while True:
		try:
			line = ser.readline().decode('ascii').strip() # skip the empty data
			timeout_cnt = timeout_cnt + 1
			if line:
			 	if (line == "Input initiator task:"):
			 	 	time.sleep(2)
			 	 	print('Input disseminate_task')
			 	 	ser.write(str(disseminate_task).encode()) # send commands
			 	if (line == "C"):
			 	 	print("*YMODEM* send\n")
			 	 	break
			if(timeout_cnt > 1000):
				break
		except:
			pass
	if(timeout_cnt > 1000):
		print("Timeout...")
		return
	# transmit the file
	transfer_to_initiator.myserial.serial_send.YMODEM_send(firmware)
	print("*YMODEM* done\n")

	# switch bank
	timeout_cnt = 0
	while True:
		try:
			line = ser.readline().decode('ascii').strip() # skip the empty data
			timeout_cnt = timeout_cnt + 1
			if line:
				print (line)
				if (line == "Input initiator task:"):
	 				time.sleep(2)
	# 				print(line)
	 				break
			if(timeout_cnt > 1000):
				break
		except:
	 		pass

	if(timeout_cnt > 1000):
		print("Timeout...")
		return