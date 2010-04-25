#include "vsx_engine.h"
#include "vsx_sequence_pool.h"

void vsx_sequence_pool::set_engine(void* new_engine)
{
	engine = new_engine;
}

int vsx_sequence_pool::add(vsx_string name) // 1 = success, 0 = fail
{
	//printf("%d\n", __LINE__);
	if (sequence_lists.find(name) != sequence_lists.end()) return 0;
	vsx_param_sequence_list* new_sequence_list = new vsx_param_sequence_list(engine);
	new_sequence_list->set_run_on_edit(false);
	sequence_lists[name] = new_sequence_list;
	//printf("%d\n", __LINE__);
	return 1;
}

int vsx_sequence_pool::del(vsx_string name)
{
	if (sequence_lists.find(name) != sequence_lists.end())
	{
		//printf("%d\n", __LINE__);
		delete sequence_lists[name];
		sequence_lists.erase(name);
		cur_sequence_list = 0;
		return 1;
	}
	return 0;
}

int vsx_sequence_pool::clone(vsx_string name, vsx_string new_name)
{
	// insanity check
	if (sequence_lists.find(name) != sequence_lists.end() && sequence_lists.find(new_name) == sequence_lists.end())
	{
		vsx_param_sequence_list* np = new vsx_param_sequence_list(*(sequence_lists[name]));
		sequence_lists[new_name] = np;//sequence_lists[name]; // thank you friendly copy constructor
		return 1; // YEAH
	}
	return 0; // BOO
}

int vsx_sequence_pool::select(vsx_string name)
{
	//printf("%d\n", __LINE__);
	if (sequence_lists.find(name) != sequence_lists.end()) {
		//printf("setting cur_sequence_list\n");
		cur_sequence_list = sequence_lists[name];
		active = name;
		return 1;
	}
	return 0;
}

vsx_param_sequence_list* vsx_sequence_pool::get_selected()
{
	return cur_sequence_list;
}

bool vsx_sequence_pool::toggle_edit()
{
	edit_enabled = !edit_enabled;

	for (std::map<vsx_string, vsx_param_sequence_list*>::iterator it = sequence_lists.begin(); it != sequence_lists.end(); it++)
	{
		(*it).second->set_run_on_edit(edit_enabled);
	}

	return edit_enabled;
}

bool vsx_sequence_pool::get_edit_enabled()
{
	return edit_enabled;
}

vsx_string vsx_sequence_pool::dump_names()
{
	vsx_string names;
	int i = 0;
	for (std::map<vsx_string, vsx_param_sequence_list*>::iterator it = sequence_lists.begin(); it != sequence_lists.end(); it++)
	{
		if (i != 0) names += ";";
		names += (*it).first;
		i++;
	}
	return names;
}

void vsx_sequence_pool::run(float dtime, bool run_from_channel)
{
	// 1. run all sequences if we're enabled
	// 2. compare all engine's hints and send the changed values up to the GUI which will then handle eventual movements
	//printf("edit enabled: %d\n",(int)edit_enabled);
	if (edit_enabled)
	{
		if (cur_sequence_list)
		{
//			printf("inner_run %f\n",vtime);
			cur_sequence_list->run_absolute(vtime);
		}
	}
}

/*void vsx_sequence_pool::run_from_channel(float vtime)
{
	if (cur_sequence_list)
	{
		set_time(vtime);
		cur_sequence_list->run(0.0f);
	}
}*/

void vsx_sequence_pool::set_time(float time)
{
	vtime = time;
  if (edit_enabled)
  {
  	//float dd = time - vtime;
  	//printf("seq pool setting time to %f\n", time);
  	//engine_info.vtime = s2f(c->parts[1]);
  	//printf("vtime: %f     %f\n",vtime,dd);
		if (cur_sequence_list) cur_sequence_list->run_absolute(vtime);
		//if (dd > 0) {
	//    engine_info.dtime = -dd;
		//} else {
//			if (cur_sequence_list) cur_sequence_list->run((float)fabs(dd));
	//    engine_info.dtime = fabs(dd);
		//}
  }
}

vsx_param_sequence_list* vsx_sequence_pool::get_sequence_list_by_name(vsx_string name)
{
	if (sequence_lists.find(name) != sequence_lists.end())
	{
		return (*(sequence_lists.find(name))).second;
	}
	return 0;
}

void vsx_sequence_pool::dump_to_command_list(vsx_command_list &savelist)
{
	for (std::map<vsx_string, vsx_param_sequence_list*>::iterator it = sequence_lists.begin(); it != sequence_lists.end(); it++)
	{
		savelist.add_raw("seq_pool add "+(*it).first);
		// 0=seq_pool 1=pseq_inject 2=[seq_pool_name] 3=[component] 4=[param] 5=[data]
		vsx_string sequence_dump = (*it).second->get_sequence_list_dump();
		vsx_string deli = "&";
		std::vector<vsx_string> parts;
		explode(sequence_dump, deli, parts);
		for (size_t i = 0; i < parts.size(); i++)
		{
			vsx_string i_deli = "#";
			std::vector<vsx_string> i_parts;
			explode(parts[i], i_deli, i_parts);
			// MULTIKRÅNGELKOMMANDO URKBURK
			savelist.add_raw("seq_pool pseq_inject "+(*it).first+" "+i_parts[0]+" "+i_parts[1]+" "+i_parts[2]);
		}
	}
//seq_pool 1=add
}


void vsx_sequence_pool::clear()
{
	cur_sequence_list = 0x0;
	for (std::map<vsx_string, vsx_param_sequence_list*>::iterator it = sequence_lists.begin(); it != sequence_lists.end(); it++)
	{
		delete (*it).second;
	}
	sequence_lists.clear();
}



vsx_sequence_pool::vsx_sequence_pool() {
	cur_sequence_list = 0;
	edit_enabled = false;
}
