import pandas as pd
import matplotlib.pyplot as plt

for load in [1, 3, 5, 10]:
    # Load the data from the 4 files into a list of dataframes
    for bg in [1, 2, 4, 8]:
        # Create a 1x11 subplot figure
        fig, axs = plt.subplots(nrows=11, ncols=1, figsize=(15, 25), gridspec_kw={"height_ratios": [2] * 11})
        fig.subplots_adjust(hspace=0.8)
        for name in ["MAB", "Q"]:
            filename = f"data/{name}-Res_T400.0_FPS90_L{load}.0_BG{bg}.0.csv"
            data = pd.read_csv(filename)
            # print(data.columns)
            # Set the simulation time as the index
            data.set_index("simtime", inplace=True)
            # Get the columns of the dataframe except for the simulation time
            cols_to_plot = data.columns[0:]
            # print(cols_to_plot)
            # Plot each column as a separate line on the current subplot
            for i, col in enumerate(cols_to_plot):
                axs[i].plot(data.index, data[col], label=f"{name} L: {load}, BG: {bg}")
                axs[i].set_title(f"Chart {data.columns[i]}")

            # Set the plot title and legend
            handles, labels = axs[0].get_legend_handles_labels()
        fig.legend(handles, labels, loc='lower center', ncol=5)
        fig.tight_layout()
        plt.savefig(f"img/plot_both_{load}_{bg}.png")
        # Show the plot
        # plt.show()
