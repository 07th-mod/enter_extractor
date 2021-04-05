import os
import subprocess
import sys
from pathlib import Path

def enter_extractor(source_path, dest_path):
	try:
		return subprocess.run(['EnterExtractor', source_path, dest_path])
	except FileNotFoundError as e:
		print("\n\n\n----------------------------------------------------------------")
		print("ERROR: Couldn't find 'EnterExtractor.exe' - Please make sure it is placed next to this python script, or on your PATH")
		print("----------------------------------------------------------------\n\n\n")
		raise e

def main(src_folder, dst_folder):
	src_paths = list(Path(src_folder).rglob('**/*.pic'))

	print(f"Processing {len(src_paths)} src images...")

	for src_path in src_paths:
		print(f"Processing {src_path}...", end='')
		src_rel_path = os.path.relpath(src_path, src_folder)
		dst_path = Path(os.path.join(dst_folder, src_rel_path)).with_suffix('.png')

		Path(os.path.dirname(dst_path)).mkdir(parents=True, exist_ok=True)
		completed_process = enter_extractor(src_path, dst_path)
		if completed_process.returncode == 0:
			print(f"Probably successful!")
		else:
			print(f"FAILED with error code {completed_process.returncode}")


if __name__ == '__main__':
	if len(sys.argv) < 3:
		print("Need 2 arguments: python enter_extractor_batch [src_folder] [dst_folder]")
		raise SystemExit(-1)

	src_folder = sys.argv[1]
	dst_folder = sys.argv[2]

	if not os.path.exists(src_folder):
		print(f"Error: Source folder \"{src_folder}\" does not exist!")
		raise SystemExit(-2)

	if not os.path.exists(src_folder):
		print(f"Warning: Destination folder \"{src_folder}\" does not exist! It will be created.")

	print(f"Updating [{src_folder}] -> [{dst_folder}]")
	main(src_folder, dst_folder)
