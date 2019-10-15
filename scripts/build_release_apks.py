#!/usr/bin/python3
import os
from pathlib import Path
import subprocess
import json
import getpass

def run_command(command,env = os.environ.copy(),print_ = False):
    command = command.split()

    result = subprocess.run(command,stdout=subprocess.PIPE,env=env)
    result = (result.returncode,result.stdout.decode('utf-8'))
    if(print_):
        print(result[1])
    return result

def get_sign_info():
    try:
        with open("sign_info.json") as f:
            sign_info = json.load(f)
    except FileNotFoundError:
        sign_info = dict()

    try:
        sign_info["path"]
    except KeyError:
        sign_info["path"] = input("Key Store Location:")

    try:
        sign_info["ks_pw"]
    except KeyError:
        sign_info["ks_pw"] = getpass.getpass(prompt='Key Store Password:')

    try:
        sign_info["alias"]
    except KeyError:
        sign_info["alias"] = input("Key Alias:")

    try:
        sign_info["key_pw"]
    except KeyError:
        sign_info["key_pw"] = getpass.getpass(prompt='Key Password:')
    return sign_info

def all_releases():
    pull_status = run_command("git pull")
    if(pull_status[0]):
        print(pull_status[1])
        print("Cannot Pull from Remote")
        exit(-1)
    releases = run_command("git tag --sort=creatordate")[1].strip().split("\n")
    return [r[1:] for r in releases if r.startswith('v')]

if __name__ == "__main__":
    releases = all_releases()
    sign_info = get_sign_info()
    cur_dir = Path(os.getcwd())
    app_ver = input("App Version:")
    if app_ver.startswith('v'):
        app_ver = app_ver[1:]
    if app_ver not in releases:
        run_command(f"git tag v{app_ver}")
        releases.append(app_ver)
    else:
        answer = ""
        while(answer not in ['Y','N']):
            answer = input("Tag already exists. Do you want to rebuild it? [Y/N]")
        if(answer != 'N'):
            exit(-1)
    release_folder = cur_dir/"release_apks"/app_ver
    game_code_folder = cur_dir/"brogue-files"
    os.makedirs(release_folder,exist_ok = True)
    git_command = f"git -C {game_code_folder}"
    versions = run_command(f"{git_command} branch -r")[1]
    versions = sorted(v.strip().replace("origin/","") for v in versions.strip().split("\n") if "/master" not in v)
    for v in versions:
        run_command(f"{git_command} checkout {v}")
        print("Compiling APK")
        version_env = os.environ.copy()
        version_env["version_code"] = str(len(releases))
        version_env["version_name"] = f"v{app_ver}"
        run_command(f"{cur_dir}/gradlew aR -p {cur_dir}",version_env)
        apk_path = f"{release_folder}/brogue-{v}-v{app_ver}.apk"
        print("Aligning APK")
        run_command(f"zipalign -v -f -p 4\
                {cur_dir}/app/build/outputs/apk/release/app-release-unsigned.apk\
                {apk_path}",print_ = True)
        print("Signing APK")
        run_command(f"apksigner sign --ks {sign_info['path']} --ks-key-alias {sign_info['alias']}\
                --ks-pass pass:{sign_info['ks_pw']} --key-pass pass:{sign_info['key_pw']} {apk_path}",print_ = True)



