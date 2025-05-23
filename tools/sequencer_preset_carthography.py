import os
import xml.etree.ElementTree as ET


def parse_xml_structure(xmlfile, sequencer_name, read_default_presets):
    # create element tree object
    tree = ET.parse(xmlfile)
    # get root element
    root = tree.getroot()

    # create empty list for news items
    preset_list = []

    # iterate news items
    for item in root.findall("songparts"):

        # iterate child elements of item
        for child in item:
            # child.tag = songpart

            desc = child.find("desc")

            preset = None

            # scan for default program
            if read_default_presets:
                program = child.find("program")
                if program is not None:
                    preset = program.text

            for patch in child.findall("patches"):
                for patch_child in patch:
                    # print(patch_child.tag, patch_child.text)
                    if patch_child.tag == sequencer_name:
                        # print("is equal")
                        preset = patch_child.text

            if preset:
                # print("  ", desc.text, preset)
                preset_list.append(preset)

    # return news items list
    return preset_list


DEFAULT_ENTRY = "....."
IGNORED_DOUBLONS = ["F15", "F16"]

if __name__ == "__main__":
    # input configuration: MC
    banks = 6
    presets_per_bank = 16
    sequencer_name = "MC"
    read_default_presets = True

    # input configuration: Digitakt
    # banks = 8
    # presets_per_bank = 16
    # sequencer_name = "Digitakt"
    # read_default_presets = True

    banks_names = ["A", "B", "C", "D", "E", "F", "G", "H"][:banks]
    carthography = [
        [DEFAULT_ENTRY for _ in range(presets_per_bank)] for _ in banks_names
    ]

    songs_dir = "../bin/data/songs"
    for song_name in os.listdir(songs_dir):
        print(" ")
        print(song_name)
        path = os.path.join(songs_dir, song_name)
        if not os.path.isdir(path):
            continue
        song_first_letters = song_name[:5]
        if len(song_first_letters) < 5:
            for _ in range(5 - len(song_first_letters)):
                song_first_letters += " "
        xml_path = os.path.join(path, "structure.xml")
        if not os.path.exists(xml_path):
            continue
        preset_used = None
        try:
            preset_used = parse_xml_structure(
                xml_path, sequencer_name, read_default_presets
            )
            print(preset_used)
        except Exception as e:
            print(e)
            continue

        if not preset_used or len(preset_used) == 0:
            continue

        for preset in preset_used:
            bank_char = preset[0]
            bank_number = banks_names.index(bank_char)
            preset_number = int(preset[1:])
            current_entry = carthography[bank_number][preset_number - 1]
            if current_entry != DEFAULT_ENTRY and current_entry != song_first_letters:
                preset_str = f"{banks_names[bank_number]}{preset_number}"
                if preset_str not in IGNORED_DOUBLONS:
                    print(
                        f"doublon detected for {preset_str}: {song_first_letters} and {current_entry}"
                    )
            carthography[bank_number][preset_number - 1] = song_first_letters

    # display carthography
    print("")
    print(
        f"     {'     '.join([str(i + 1).zfill(5) for i in range(presets_per_bank)])}"
    )
    for idx, bank_name in enumerate(banks_names):
        print(f"{bank_name}:   {'     '.join(carthography[idx])}")
