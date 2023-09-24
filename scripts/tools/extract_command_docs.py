import os
import sys
import json
import rst_to_myst
import docutils
import docutils.core

commands_doc_path = os.getenv("SIOYEK_DOCUMENTATION_COMMANDS_PATH")

def export_documentation(commands, documentation_for_titles, dest_file):
    res = dict()

    for command_list, doc in zip(commands, documentation_for_titles):

        doc_html = docutils.core.publish_parts(doc, writer_name='html')['html_body']
        # doc_markdown = rst_to_myst.rst_to_myst(doc).text
        # doc_markdown = doc_markdown.replace("{code}", "")
        
        for command in command_list:
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
        return "_".join(parts[:-1]) + "*"
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
    

if __name__ == '__main__':
    lines = []
    with open(commands_doc_path, 'r') as infile:
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
    
    all_documented_commands = sum(commands, [])
    all_commands = get_all_command_names()
    all_commands_translated = [translate_command_name(x) for x in all_commands]
    
    if len(sys.argv) > 1:
        arg = sys.argv[1]
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
            command_name = sys.argv[2]
            print(get_doc_for_command(commands, documentation_for_titles, command_name))
        if arg == "export":
            dest_file = sys.argv[2]
            export_documentation(commands, documentation_for_titles, dest_file)
    # print(sys.argv)
    # for command in commands:
    #     print(command)
    # for title, doc in zip(titles, documentation_for_titles):
    #     print(title)
    #     print('_______________')
    #     print(documentation_for_titles)
    #     print('_______________')
    # print(get_doc_for_command(commands, documentation_for_titles, "copy_window_size_config"))
    # print(get_all_command_names())

    # doc = get_doc_for_command(commands, documentation_for_titles, "overview_under_cursor")
    # print(doc)
    # all_commands = list(set(get_all_command_names()))
    # undocumented_commands = list(set(get_undocumented_commands(all_documented_commands, all_commands)))
    # for x in undocumented_commands:
    #     print(x)