/**
  * <p>Преобразование xml-сэмплов ОКБ в человеческие xml</p>
  * <p>На вход input.xml - сэмпл ОКБ;
  * Работать при условии, если в строке не больше одного открываюего и закрывающего тега,
  * если нужен более общий случай - это легко допилить (дополнив перечень состояний автомата), но в задачу это не входило.</p>
  */

#include "stdafx.h"
#include "fstream"
#include "string"
#include "list"
using namespace std;

class okb_machine {
	private:
		//Перечень возможных состояний автомата
		enum statespace {
			_BEGIN_OF_STRING,
			_FOUND_START_TAG,
			_FOUND_TAG_TYPE,
			_FOUND_TAG_NAME,
			_CLOSE_TAG_NAME,
			_CLOSE_START_TAG,
			_FOUND_END_TAG,
			_CLOSE_END_TAG,
			_FOUND_CLOSE_TAG
		};
		
		//Виды xml-тэгов
		enum tagtype {
			_SERVICE,
			_ATTRIBUTE_NILLABLE,
			_ATTRIBUTE,
			_COLLECTION,
			_STRUCTURE,
			_STRUCTURE_NAMED,
			_EOF_ATTRIBUTE,
			_EOF_COLLECTION,
			_EOF_STRUCTURE
		};
		
		//Для хранения вложенных структур
		class tag {
			public :
				tagtype	tag_type;												//Классификация тэгов
				string	tag_name;												//Имя тега
				string	tag_value;												//Значение тэга
				
				//Конструктор по умолчанию
				tag() {
					tag_name  = "";
					tag_value = "";
				}

				//Конструктор, принимающий имя и тиг тэга
				tag(tagtype init_type, string init_name) {
					tag_type  = init_type;
					tag_name  = init_name;
				}
				
				//Изменить имя тэга
				void setName(string new_name) {
					tag_name  = new_name;
				}

				//Изменить имя
				void setValue(string new_value) {
					tag_value  = new_value;
				}

				//Изменение типа тега
				void setType(tagtype new_type) {
					tag_type=new_type;
				}

				//Сверка типа тега
				bool checkType(tagtype type) {
					return tag_type==type;
				}
		};

		statespace	state;																//Текущее состояние автомата
		tag			current_tag;														//Текущий тэг
		string		first_spaces;														//Строка-отступ
		int			str_num;
		
		//Переход в следующее состояние конечного автомата. Возвращает true, если есть смысл продолжать чтение строки.
		bool next_state(char c) {
			switch (state) {
				case _BEGIN_OF_STRING:
					switch (c) {
						case '<':	state=_FOUND_START_TAG;
									break;

						default :	first_spaces+=" ";
									break;
					}
					break;

				case _FOUND_START_TAG:
					switch(c) {
						case 'a':	current_tag.setType(_ATTRIBUTE);
									state			= _FOUND_TAG_TYPE;
									break;

						case 's':	current_tag.setType(_STRUCTURE);
									state			= _FOUND_TAG_TYPE;
									break;

						case 'c':	current_tag.setType(_COLLECTION);
									state			= _FOUND_TAG_TYPE;
									break;

						case '?':	current_tag.setType(_SERVICE);
									return false;

						case '/':	state			= _FOUND_CLOSE_TAG;
									break;
					}
					break;

				case _FOUND_CLOSE_TAG:
					switch(c) {
						case 'a':	current_tag.setType(_EOF_ATTRIBUTE);
									return false;

						case 's':	current_tag.setType(_EOF_STRUCTURE);
									return false;

						case 'c':
						case '>':	current_tag.setType(_EOF_COLLECTION);
									return false;
					}
					break;

				case _FOUND_TAG_TYPE:
					switch(c) {
						case '"':	state = _FOUND_TAG_NAME;
									if (current_tag.checkType(_STRUCTURE)) {
										current_tag.setType(_STRUCTURE_NAMED);
									}
									break;

						case '>':	return false;
					}
					break;

				case _FOUND_TAG_NAME:
					switch(c) {
						case '"':	state = _CLOSE_TAG_NAME;
									if (current_tag.checkType(_COLLECTION) || current_tag.checkType(_STRUCTURE_NAMED)) {
										return false;
									}
									break;

						default :	current_tag.tag_name+=c;
									break;
					}
					break;
				
				case _CLOSE_TAG_NAME:
					switch(c) {
						case '>':	state			= _CLOSE_START_TAG;
									break;

						case '/':	current_tag.setType(_ATTRIBUTE_NILLABLE);
									return false;
					}
					break;

				case _CLOSE_START_TAG:
					switch(c) {
						case '<':	state			= _FOUND_END_TAG;
									break;

						default :	current_tag.tag_value+=c;
									break;
					}
					break;
			}			
			return true;
		} 
	
		//Определяет, нужно ли отображать пробелы в начале строки (Структурированный xml-формат/все в одну строку)
		bool isShowSpaces(bool use_spaces) {
			return (use_spaces && (current_tag.checkType(_ATTRIBUTE_NILLABLE)						||
								   current_tag.checkType(_ATTRIBUTE ) && current_tag.tag_name!=""	||
								   current_tag.checkType(_STRUCTURE)								||
								   current_tag.checkType(_STRUCTURE_NAMED)							||
								   current_tag.checkType(_EOF_STRUCTURE)
								  )
				   );
		}

	public:
		//Конструктор по умолчанию
		okb_machine() {
		}

		//Основная функция по преобразованию xml в нормальный вид
		bool transform_xml(string filename_input, string filename_output, string core_node_name, string error_message, bool use_spaces) {
			ifstream input(filename_input);												//Входные/выходные файлы
			ofstream output(filename_output);
			string cur_str;

			list<tag> node_names;														//Хранение названий вложенных структур
			tag temp_container(_COLLECTION, core_node_name);
			node_names.push_back(temp_container);

			while (getline(input,cur_str)&&++str_num) {
				if (state!=_CLOSE_START_TAG) {											//Если открывающий и закрывающий тэги атрибута не расположены в разных строках 
					state=_BEGIN_OF_STRING;
					current_tag.setName("");
					current_tag.setValue("");
					first_spaces="";
				}

				for (int i=0; (i<cur_str.length() && next_state(cur_str[i])); i++);		//Получаем из входной строки параметры тэга
				
				if (state==_CLOSE_START_TAG) {
					continue;
				}

				if (!node_names.size())	{												//Единственный эксепшн
					output<<endl<<error_message;
					output.close();
					return false;
				}

				if (isShowSpaces(use_spaces)) {
					output<<first_spaces;
				}

				switch (current_tag.tag_type) {
					case _SERVICE:				output<<cur_str;
												break;

					case _ATTRIBUTE_NILLABLE:	output<<"<"+current_tag.tag_name+"/>";
												break;
					
					case _ATTRIBUTE:			if (current_tag.tag_name!="") {
													if (current_tag.tag_value=="")
														output<<"<"+current_tag.tag_name+"/>";
													else 
														output<<"<"+current_tag.tag_name+">"+current_tag.tag_value+"</"+current_tag.tag_name+">";
												}
												break;

					case _COLLECTION:			node_names.push_back(current_tag);
												break;

					case _EOF_COLLECTION:		node_names.pop_back();
												break;

					case _STRUCTURE:			output<<+"<"+node_names.back().tag_name+">";
												break;

					case _STRUCTURE_NAMED:		node_names.push_back(current_tag);
												output<<+"<"+node_names.back().tag_name+">";
												break;

					case _EOF_STRUCTURE:		switch (node_names.back().tag_type) {
													case _COLLECTION	:	output<<+"</"+node_names.back().tag_name+">";
																			break;
												
													default				:	output<<+"</"+node_names.back().tag_name+">";
																			node_names.pop_back();
																			break;
												}
												break;
				}
				
				if (isShowSpaces(use_spaces)) {
					output<<endl;
				}

			}

			input.close();
			output.close();
			return true;
		}
};

int _tmain(int argc, _TCHAR* argv[])
{
	string	filename_input	= "input.xml";
	string	filename_output	= "output.xml";
	string	core_node_name	= "EnquiryResponse";
	string	error_message	= "XML невалидна";
	bool	use_spaces		= true;
	
	okb_machine machine;
	machine.transform_xml(filename_input, filename_output, core_node_name, error_message, use_spaces);
	return 0;
}