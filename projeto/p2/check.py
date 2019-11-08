import os, pickle, sys

def load_obj(name ):
    with open(name + '.pkl', 'rb') as f:
        return pickle.load(f)
    
def main():
    input_file = open(sys.argv[1], "r") #temp
    buffer=input_file.read()
    input_file.close()
    test_number=buffer[4:-4:1]
    os.remove(sys.argv[1])

    os.chdir("myinp")
    inumbers = load_obj("test"+str(test_number)+".dict")
    test_file = open("test"+str(test_number)+".stdout", "r")
    print(inumbers)
    while(True):
        k = test_file.readline()
        k = k.split(" ")
        print(k)
        if k[0] == "TecnicoFS":
            break
        if " " + k[0] in inumbers.keys():
            print(k[-1][:-1:])
            print(str(inumbers[" " + k[0]][0]))
            if k[-1] != (str(inumbers[" " + k[0]][0])+'\n') and k[-1] != "found\n":
                print("FAILED")
                return 1
        else:
            if k[-1] != "found\n":
                print("FAILED")
                return 1
    print("SUCSESS")
    return 0
main()