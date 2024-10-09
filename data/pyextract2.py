import pandas as pd
import plotly.express as px
import os
import click
import sys

@click.command()
@click.option("--b",is_flag=True,help='binary view')
@click.option("--g",is_flag=True,help='graph byte visualisation')
@click.option("--file_n",type=int,help='select file, -1 for newest')
@click.option("--filelist",is_flag=True,help='file list')
def execute(b,g,file_n,filelist):
    current_file_path = os.path.abspath(__file__)
    root_dir = os.path.dirname(current_file_path)
    data_path = os.path.join(root_dir,f'datatest/')

    print(f"The directory of the current file is: {root_dir}")

    
    files = os.listdir(data_path)
    print(f'you have {files} files')

    if filelist:
        for f in files:
            print(f)
        sys.exit(0)

    #get largest index
    largest_n = 0
    for f in files:
        if int(f.split(".")[0])> largest_n:
            largest_n = int(f.split(".")[0])

    #imidiatly add file to index
    old_fp = os.path.join(root_dir,'0.log')
    if os.path.exists(old_fp):
        new_fp = os.path.join(data_path,f'{largest_n+1}.log') 
        os.system(f'cp {old_fp} {new_fp}')
        print('added new file to index',largest_n+1)
        
        if os.path.exists(new_fp):
            try:
                os.unlink(old_fp)
            except:
                pass


    #make sure when you monitor the esp deivce you save it to /data/0.log
    if file_n==-1:
        fp = os.path.join(data_path,f'{largest_n+1}.log') 
    
    if file_n>-1:
        fp = os.path.join(data_path,f'{file_n}.log') 

    if not os.path.exists(data_path):
        print(f'please make the dir {data_path}')
        sys.exit(0)


    chip_type = None
    max_task_wait = None
    address_interval = None
    virtual_chain_length = None
    chips_detected = None
    hcn = None
    chip_diff= None
    print('extract experiment data from log')
    with open(fp,mode='r') as file:
        lines = file.readlines()
        for line in lines:
            if 'CHIP ' in line:
                print(line)
                key = line.split(' ')[4]
                value = line.split(' ')[5].replace("\x1b[0m\n","")

                if key == "type":
                    chip_type = value
                if key == "max_task_wait":
                    max_task_wait = int(value)

                if key == "address_interval":
                    address_interval = int(value)
                
                if key == "virtual_chain_length":
                    virtual_chain_length = int(value)

                if key == "chips_detected":
                    chips_detected = int(value)
                
                if key == "hcn":
                    hcn = value

                if key == "chip_diff":
                    chip_diff = int(value)


            if None in [hcn,chips_detected,virtual_chain_length,address_interval,max_task_wait,chip_type,chip_diff]:
                continue
            
            else:
                break
        
    if None in [hcn,chips_detected,virtual_chain_length,address_interval,max_task_wait,chip_type,chip_diff]:
        print('esp miner does not have variables required for analysis')
        sys.exit(0)

    

    data  = []
    b_data = []
    hashrate = 0
    nonce_count = 0
    with open(fp,mode='r') as file:
        lines = file.readlines()
        for line in lines:
            if 'asic_result: Ver' in line:
                time = int(line.split(' ')[1][1:-1])
                version = int(line.split(' ')[4].replace(",",""),16)
                version_hex = hex(version>> 16)[2:].zfill(8)
                #version_hex2 = hex(version>> 16)[2:].zfill(8)

                nonce = int(line.split(' ')[6].zfill(8),16)
                nonce_hex = line.split(' ')[6].zfill(8)
                n3 = int(nonce_hex[0:2],16)
                n2 = int(nonce_hex[2:4],16)
                n1 = int(nonce_hex[4:6],16)
                n0 = int(nonce_hex[6:8],16)

                v0 = int(version_hex[0:2],16)
                v1 = int(version_hex[2:4],16)
                v2 = int(version_hex[4:6],16)
                v3 = int(version_hex[6:8],16)
                
                nonce_int_rev = n0*255*255*255 + n1*255*255 + n2*255 + n3
                
                bstring_nonce = (
                    bin(n0)[2:].zfill(8)+' '+bin(n1)[2:].zfill(8)+' '+bin(n2)[2:].zfill(8)+' '+bin(n3)[2:].zfill(8) + ' ' +
                    bin(v1)[2:].zfill(8)+' '+bin(v2)[2:].zfill(8)+' '+bin(v3)[2:].zfill(8) + " " + bin(version>> 13)[2:][-3:]
                )
                

                data.append([time,version,nonce,n0,n1,n2,n3,v0,v1,v2,v3])
                hashrate = chip_diff*len(data)*2**32/time/1000000

                if b: print(bstring_nonce,version,version_hex)
                nonce_count+=1
    if hashrate==0:
        print('no nonces discovered abort')
        sys.exit(0)

    print('hashrate',hashrate,time,len(data))
    print('hcn',hcn,int(hcn,16))
    print('max_task_wait',max_task_wait)
    print('virtual_chain_length',virtual_chain_length)   
    print('address_interval',address_interval) 
    print('max_task_wait',max_task_wait) 
    print('chip_type',chip_type) 
    print('chip_diff',chip_diff) 

    if g:      
        frame = pd.DataFrame(data,columns=['time','version','nonce','n0','n1','n2','n3','v0','v1','v2','v3'])
        nframe = pd.melt(frame,id_vars=['time','version','nonce'],value_vars=['n0','n1','n2','n3'],var_name='byte',value_name='value')

        px.scatter(nframe,x='time',y='value',color='byte',hover_data=['version','nonce'],marginal_y='histogram').show()

        vframe = pd.melt(frame,id_vars=['time','version','nonce'],value_vars=['v0','v1','v2','v3'],var_name='byte',value_name='value')
        px.scatter(vframe,x='time',y='value',color='byte',hover_data=['version','nonce'],marginal_y='histogram').show()

    


if __name__ == "__main__":
    execute()