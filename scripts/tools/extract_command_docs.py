import os
import sys
import json
import rst_to_myst
import docutils
import docutils.core

commands_doc_path = os.getenv("SIOYEK_DOCUMENTATION_COMMANDS_PATH")
config_doc_path = os.getenv("SIOYEK_DOCUMENTATION_CONFIGS_PATH")

def export_documentation(commands, documentation_for_titles, dest_file, translate=False):
    res = dict()

    for command_list, doc in zip(commands, documentation_for_titles):

        doc_html = docutils.core.publish_parts(doc, writer_name='html')['html_body']
        # doc_markdown = rst_to_myst.rst_to_myst(doc).text
        # doc_markdown = doc_markdown.replace("{code}", "")
        
        for command in command_list:
            if translate:
                command = translate_command_name(command)
            res[command] = doc_html

    with open(dest_file, "w") as outfile:
        json.dump(res, outfile)

def get_doc_for_command(doc_commands, documentations, command_name):
    command_name = translate_command_name(command_name)
    for i, commands in enumerate(doc_commands):
        if command_name in commands:
            return documentations[i]
    return None

def translate_command_name(command_name):
    parts = command_name.split("_")
    if command_name.startswith("setconfig"):
        return "setconfig_*"
    if command_name.startswith("toggleconfig_"):
        return "toggleconfig_*"

    if len(parts[-1]) == 1 and parts[-1] != "*":
        return "_".join(parts[:-1]) + "_*"
    else:
        return command_name

def get_undocumented_commands(documented_command_names, all_command_names):
    return list(set(all_command_names) - set(documented_command_names))

def get_commands_from_title(title_text):
    commands = []
    while title_text.find(":code:") != -1:
        index = title_text.find(":code:")
        title_text = title_text[index + len(":code:") + 1:]
        title_end_index = title_text.find('`')
        commands.append(title_text[:title_end_index])
        title_text = title_text[title_end_index+1:]
    return commands

def get_all_command_names(include_macro: bool = False):
    command_names_file_path = "command_names.txt"
    names = []
    with open(command_names_file_path, 'r') as infile:
        for line in infile:
            if line[0] != '_' or include_macro:
                names.append(translate_command_name(line.strip()))
    return names

def get_all_config_names():
    command_names_file_path = "config_names.txt"
    names = []
    with open(command_names_file_path, 'r') as infile:
        for line in infile:
            names.append(translate_command_name(line.strip()))
    return names
    
def parse_documentation_file(file_path):
    lines = []
    with open(file_path, 'r', encoding='utf8') as infile:
        for line in infile:
            lines.append(line)
    
    titles = []
    documentation_for_titles = []
    last_title_line_index = -1

    lines.append("^^^^^^^")
    for i, line in enumerate(lines):
        if len(line) > 0 and line[0] == '^':
            if last_title_line_index != -1:
                documentation_for_titles.append("".join(lines[last_title_line_index-1: i-1]).strip())
            titles.append(lines[i-1].strip())
            last_title_line_index = i
    
    commands = []
    for title in titles:
        commands.append(get_commands_from_title(title))
    
    return commands, documentation_for_titles

if __name__ == '__main__':
    
    documented_commands, command_documentations = parse_documentation_file(commands_doc_path)
    documented_configs, config_documentations = parse_documentation_file(config_doc_path)

    all_documented_commands = sum(documented_commands, [])
    all_commands = get_all_command_names()
    all_commands_translated = [translate_command_name(x) for x in all_commands]

    all_documented_configs = sum(documented_configs, [])
    all_configs = get_all_config_names()
    all_configs_translated = [translate_command_name(x) for x in all_configs]
    
    if len(sys.argv) > 1:
        mode = sys.argv[1]
        if mode == "command":
            arg = sys.argv[2]
            if arg == 'all':
                for command in all_commands:
                    print(command)
            if arg == 'documented':
                for command in all_documented_commands:
                    print(command)
            if arg == 'undocumented':
                undocumenteds = list(set(all_commands_translated) - set(all_documented_commands))
                for undocumented_command in undocumenteds:
                    print(undocumented_command)
            if arg == "get":
                command_name = sys.argv[3]
                print(get_doc_for_command(documented_commands, command_documentations, command_name))
            if arg == "export":
                dest_file = sys.argv[3]
                export_documentation(documented_commands, command_documentations, dest_file)
        if mode == "config":
            arg = sys.argv[2]
            if arg == 'all':
                for config in all_configs:
                    print(config)
            if arg =='documneted':
                for config in all_documented_configs:
                    print(config)
            if arg == 'undocumented':
                undocumenteds = list(set(all_configs_translated) - set(all_documented_configs))
                for undocumented_config in undocumenteds:
                    print(undocumented_config)
            if arg == 'get':
                config_name = sys.argv[3]
                print(get_doc_for_command(documented_configs, config_documentations, config_name))
            if arg == "export":
                dest_file = sys.argv[3]
                export_documentation(documented_configs, config_documentations, dest_file, translate=True)
        if mode == "debug":
            pass