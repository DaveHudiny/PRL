import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns

def plots():
    pass

def mess():
    with open("results.txt", "r") as file:
        dictionary = {}
        for line in file.readlines():
            splitline = line.split(" ")
            dictionary[splitline[0]] = []
            for measure in splitline[1:-1]:
                dictionary[splitline[0]].append(measure)
        pdf = pd.DataFrame(dictionary)
        pdf = pdf.T
        print(pdf)
        sns.boxplot(data=pdf, min=0, max=1)


if __name__ == "__main__":
    pdf = pd.read_csv("results.txt", sep=' ', index_col="Index")
    
    pdf = pdf.iloc[::4]
    ax = sns.boxplot(data=pdf.T)
    ax.set(xlabel = "Počet prvků/procesorů", ylabel = "Délka běhu [ms]")
    plt.show()