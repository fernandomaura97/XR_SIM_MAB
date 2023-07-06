
% Define the filename pattern
% fileNamePattern = 'MAB-_S%d_T400_FPS90_L1_BG%dNbg%d.csv';
clc
clear all
close all

%%%% options: For RTT or Load %%%%%%%%%%%%%
% options = 'load';
% options = 'RTT' 
% options = 'frameloss' %%TODO
options = 'hist'
% options = 'cumr'
%%%%

if strcmp(options, 'cumr') 
    cumr_mean = zeros(8,4,5, 400); 
end

% locs = {'Greedy_MAB', 'Greedy_neighbors', 'Qlearning', 'Qlearning_n2', 'Softmax', 'Thompson_b', 'Thompson_g', 'UCB'};
% names = {'MAB', 'MABN', 'Q_N1_', 'Q_N2_', 'SOFT', 'TS_BETA', 'THMPSN', 'UCB'};

locs = {'Greedy_MAB', 'Greedy_neighbors', 'Softmax', 'UCB'};
names = {'MAB', 'MABN', 'SOFT', 'UCB'};


dict = containers.Map(locs, names);

% Access the dictionary elements
keys = dict.keys();     % Retrieve all keys
values = dict.values(); % Retrieve all values


tilelay2 = tiledlayout(4,4, "TileSpacing","compact");

    

Main_title = sprintf("Histogram of loads comparison");
% set(fig, 'UserData', TITLE);
title(tilelay2, Main_title)

for i_orig = 1:numel(keys)  %% i_orig is algorithm_index
    disp(['Key: ', keys{i_orig}, ', Value: ', values{i_orig}]);

    TITLE = values{i_orig};
    filePath_orig = ['/home/boris/MATLAB/XR_SIM/res/' keys{i_orig}];
    

    test_var = sprintf("%s", filePath_orig)
    filePath = filePath_orig; %%just some shenanigans to keep track f the original var without modifying more code
    
    fileNamePattern = TITLE + "_S%d_T400_FPS90_L1_BG%dNbg%d.csv"
    if strcmp(TITLE, 'Q_N1_') 
        fileNamePattern = "Q-N1_" + "_S%d_T400_FPS90_L1_BG%dNbg%d_a%.2f_g%.2f_Tu%.2f.csv"
    end
    if strcmp(TITLE, 'Q_N2_')
        fileNamePattern = "Q-N2_" + "_S%d_T400_FPS90_L1_BG%dNbg%d_a%.2f_g%.2f_Tu%.2f.csv"
    end

    % Get a list of all files in the directory
    fileList = dir(filePath); % Update the directory path accordingly
    cd(filePath); %%enter the working directory
    
    % Initialize the outer struct to store the nested structs
    outerStruct = struct();
    
    % Iterate over each file in the directory
    for i_list = 1:numel(fileList)
        if(size(fileList(i_list).name,2)>25)

        filename = fileList(i_list).name;
        
        % Match the filename pattern
    
            match = sscanf(filename, fileNamePattern);
            matches = regexp(filename, sprintf(fileNamePattern), 'tokens')
            % Check if the filename matches the pattern
            if numel(match) == 3
                S = match(1);
                BG = match(2);
                Nbg = match(3);
    
                % if(S==1) 
                
                    % Create the nested struct field using the values of "BG" and "Nbg"
                    nestedPrefix = sprintf('%s_BG_%d__Nbg_%d',TITLE, BG, Nbg);
                    
                    % Check if the nested struct field already exists
                    if isfield(outerStruct, nestedPrefix)
                        innerStruct = outerStruct.(nestedPrefix);
                    else
                        % Create a new nested struct if it doesn't already exist
                        innerStruct = struct();
                    end
                    
                    % Create the struct field using the value of "S%d"
                    prefix = sprintf('S%d', S);
                    
                    % Check if a struct field already exists for the prefix within the nested struct
                    if isfield(innerStruct, prefix)
                        % Append the table to the existing field within the nested struct
                        innerStruct.(prefix) = [innerStruct.(prefix); readtable(filename)];
                    else
                        % Create a new field within the nested struct and assign the table
                        innerStruct.(prefix) = readtable(filename);
                    end
                    
                    % Update the nested struct within the outer struct
                    outerStruct.(nestedPrefix) = innerStruct;
                % end
            end
        end
    end
  
    fields = fieldnames(outerStruct);

    %%aux_vars for boxplots
    matrix_cumsim = zeros(numel(fields), numel(fieldnames(outerStruct.(fields{1}))),4);
    
    for k = 1:numel(fields)                                     %% for each N_BG scenario
        sub_fields = fieldnames(outerStruct.(fields{k}));
        data_v = {};
         a = sprintf("%s",fields{k});
        % title = a;
         nexttile;
        
       
        % disp(sub_fields{1})
        for i=1:numel(sub_fields)   
            hold on
            data = outerStruct.(fields{k}).(sub_fields{i}); %% ONLY ONE SEED IN THIS CASE
            % head(outerStruct.(fields{k}).(sub_fields{i}))
           
            
            if(strcmp(options,'RTT'))  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                xData = data.simtime;
            
                ydata = data.RTT;
                plot(xData, smoothdata(ydata));
                xlabel('t')
                ylim([0 0.5])
                ylabel('RTT [s]')
    
            elseif (strcmp(options,'load')) %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                xData = data.simtime;
            
                ydata = data.load;
                plot(xData, ydata);
                xlabel('t')
                ylim([0 100E6])
                ylabel('Load [bps]')
            
            elseif strcmp(options, 'hist')
                histogram(data.load / 10E5,10, 'FaceAlpha', 0.4, 'BinLimits',[0 100]);
                ylim([0 400])
                xlabel('Mbps', 'FontSize',8)
                
                title([TITLE, num2str(S) ]);

            elseif strcmp(options, 'cumr')
                
                cumr_mean(i_orig, k, i, :) = data.cum_reward;
                
                if i == numel(sub_fields) %% if all seeds complete
                % Reshape the cumulative rewards data into a 2D matrix for box plot
                    cumr_data = reshape(cumr_mean(i_orig, k, :, :), [], numel(sub_fields));
                
                    % Create a subplot for the box plot                    
                    % Generate the box plot
                    boxplot(cumr_data);
                    ylabel('Cumulative Reward');
                    title(dict(keys{i_orig}));
                
                    % Adjust the plot settings as needed
                    % For example, you can set the ylim, grid, etc.
                    ylim([0, max(cumr_mean(:))]); % Adjust the y-axis limits if needed
                    grid on;
                end
                            
                % plot(data.simtime, smoothdata(data.reward, 5))
                % ylim([0 1])               
            end

            if strcmp(TITLE, 'Q_N1_') 
                a = sprintf("Q-learning (Neighbors \pm1): 1dBG")                  
             end
            if strcmp(TITLE, 'Q_N2_')
                a = sprintf("Q-learning (Neighbors \pm2): 1dBG")                
            end

            switch TITLE
                case 'MAB'
                    title_good = "\epsilon-greedy"
                case 'MABN'
                    title_good = "\epsilon-greedy(Neighbors)"
                case 'Q-N1_'
                    title_good = 'Q-learning (\pm1)'
                case 'Q-N2_'
                   title_good = 'Q-learning (\pm2)';
                case 'SOFT'
                   title_good = '\epsilon-greedy (Softmax)';
                case 'TS_BETA'
                   title_good = 'Thompson (\beta)';              
                case 'THMPSN', 
                   title_good = 'Thompson (\sigma) ';
                case 'UCB';
                    title_good = 'UCB1';
                otherwise
                    fprintf("Error, no name given??\n");
            end
            title(title_good, 'FontSize',9)
            shg
            
        end
        hold off
    end
    
    figHandles = findobj('Type', 'figure');
    
    savePath = sprintf("/home/boris/MATLAB/XR_SIM/res/aa_PLOTS_IMGS/%s/%s_%s", options, options, TITLE);
    sv_fig = [savePath + "/fig"]
    % Create the directory if it doesn't exist
    if ~exist(savePath, 'dir')
        mkdir(savePath);
    end
    if ~exist(sv_fig, 'dir')
        mkdir(sv_fig);
    end
    % 
    % % Loop through each figure and save it
    % for i = 1:numel(figHandles)
    %     % Get the figure's title
    %     titleStr = get(figHandles(i), 'UserData');
    %     % disp("titstr" + titleStr)
    % 
    %     % Remove any invalid characters from the title
    %     filename = regexprep(titleStr, '[^\w\-_.() ]', '_');
    % 
    %     % disp("FN:" + filename )
    %     % Set the full file path for saving
    %     filePath = fullfile(savePath, [filename '.png']);
    %     filePath_fig = fullfile(sv_fig, [filename '.fig']);
    % 
    % 
    %     % Save the figure as PNG using the saveas function
    %     saveas(figHandles(i), filePath, 'png');
    %     saveas(figHandles(i), filePath_fig, 'fig');
    % 
    % 
    % end
end

