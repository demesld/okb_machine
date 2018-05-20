/**
  * <p>�������������� xml-������� ��� � ������������ xml</p>
  * <p>�� ���� input.xml - ����� ���;
  * �������� ��� �������, ���� � ������ �� ������ ������ ����������� � ������������ ����,
  * ���� ����� ����� ����� ������ - ��� ����� �������� (�������� �������� ��������� ��������), �� � ������ ��� �� �������.</p>
  */

#include "stdafx.h"
#include "fstream"
#include "string"
#include "list"
using namespace std;

class okb_machine {
	private:
		//�������� ��������� ��������� ��������
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
		
		//���� xml-�����
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
		
		//��� �������� ��������� ��������
		class tag {
			public :
				tagtype	tag_type;												//������������� �����
				string	tag_name;												//��� ����
				string	tag_value;												//�������� ����
				
				//����������� �� ���������
				tag() {
					tag_name  = "";
					tag_value = "";
				}

				//�����������, ����������� ��� � ��� ����
				tag(tagtype init_type, string init_name) {
					tag_type  = init_type;
					tag_name  = init_name;
				}
				
				//�������� ��� ����
				void setName(string new_name) {
					tag_name  = new_name;
				}

				//�������� ���
				void setValue(string new_value) {
					tag_value  = new_value;
				}

				//��������� ���� ����
				void setType(tagtype new_type) {
					tag_type=new_type;
				}

				//������ ���� ����
				bool checkType(tagtype type) {
					return tag_type==type;
				}
		};

		statespace	state;																//������� ��������� ��������
		tag			current_tag;														//������� ���
		string		first_spaces;														//������-������
		int			str_num;
		
		//������� � ��������� ��������� ��������� ��������. ���������� true, ���� ���� ����� ���������� ������ ������.
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
	
		//����������, ����� �� ���������� ������� � ������ ������ (����������������� xml-������/��� � ���� ������)
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
		//����������� �� ���������
		okb_machine() {
		}

		//�������� ������� �� �������������� xml � ���������� ���
		bool transform_xml(string filename_input, string filename_output, string core_node_name, string error_message, bool use_spaces) {
			ifstream input(filename_input);												//�������/�������� �����
			ofstream output(filename_output);
			string cur_str;

			list<tag> node_names;														//�������� �������� ��������� ��������
			tag temp_container(_COLLECTION, core_node_name);
			node_names.push_back(temp_container);

			while (getline(input,cur_str)&&++str_num) {
				if (state!=_CLOSE_START_TAG) {											//���� ����������� � ����������� ���� �������� �� ����������� � ������ ������� 
					state=_BEGIN_OF_STRING;
					current_tag.setName("");
					current_tag.setValue("");
					first_spaces="";
				}

				for (int i=0; (i<cur_str.length() && next_state(cur_str[i])); i++);		//�������� �� ������� ������ ��������� ����
				
				if (state==_CLOSE_START_TAG) {
					continue;
				}

				if (!node_names.size())	{												//������������ �������
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
	string	error_message	= "XML ���������";
	bool	use_spaces		= true;
	
	okb_machine machine;
	machine.transform_xml(filename_input, filename_output, core_node_name, error_message, use_spaces);
	return 0;
}