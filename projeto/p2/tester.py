import os, random, pickle, sys
max_commands = 0#150000
comandos = ['c', 'l', 'd', 'r']
nomes=[]
inumbers={}
contador=[1]

def rand_cmd():#c l d r
    comando = random.randint(0, len(comandos)*2)
    if comando >= len(comandos) and nomes:
        name = nomes[random.randint(0, len(nomes)-1)]
    else:
        name = rand_name() 
    name2 = "" if (comandos[comando%len(comandos)] != 'r') else rand_name()

    if(comandos[comando%len(comandos)]=='c'):
        if name not in inumbers.keys():
            inumbers[name]=[contador[0]]
            contador[0]+=1
    if(comandos[comando%len(comandos)]=='d'):
        if name in inumbers.keys():
            del inumbers[name]
    if(comandos[comando%len(comandos)]=='r'):
        if name in inumbers.keys():
            inumbers[name2]=inumbers[name][::]
            del inumbers[name]
    return str(comandos[comando%len(comandos)]) + name + name2 + '\n'

def rand_name():
    ran=(ord('z')-ord('a'))+(ord('Z')-ord('A'))
    res=' '
    for i in range(45):
        char=random.randint(0, ran)
        if char <= (ord('Z')-ord('A')):
            #print(char + ord('A'))
            res += chr(char + ord('A'))
        else:
            #print(char + ord('a') - (ord('Z')-ord('A'))-1)
            res += chr(char + ord('a') - (ord('Z')-ord('A'))-1)
    nomes.append(res)
    return res

def save_obj(obj, name ):
    with open(name + '.pkl', 'wb') as f:
        pickle.dump(obj, f, pickle.HIGHEST_PROTOCOL)


def main():
    max_commands=int(sys.argv[1])
    inputs = os.listdir("inputs")
    k="0"
    for files in inputs:
        k = files[4:-4:] if files[4:-4:]>k else k
    os.chdir("myinp")
    output_file = open("test" + str(int(k)+1) + ".txt","w")
    for i in range(max_commands):
        output_file.write(str(rand_cmd()))
    
    output_file.flush()
    output_file.close()
    save_obj(inumbers, "test" + str(int(k)+1) + ".dict")
    temp = open("temp", "w")
    temp.write("test" + str(int(k)+1) + ".txt")
    temp.close()
main()