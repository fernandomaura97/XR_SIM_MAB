
% Define the filename pattern
% fileNamePattern = 'MAB-_S%d_T400_FPS90_L1_BG%dNbg%d.csv';
clc
clear all
close all

%%%% options: For RTT or Load %%%%%%%%%%%%%
% options = 'load';
options = 'RTT' 
% options = 'frameloss' 
% options = 'hist'

%%%%

locs = {'Greedy_MAB', 'Greedy_neighbors', 'Qlearning', 'Qlearning_n2', 'Softmax', 'Thompson_b', 'Thompson_g', 'UCB'};
names = {'MAB', 'MABN', 'Q-N1_', 'Q-N2_', 'SOFT', 'TS_BETA', 'THMPSN', 'UCB'};
dict = containers.Map(locs, names);

% Access the dictionary elements
keys = dict.keys();     % Retrieve all keys
values = dict.values(); % Retrieve all values
for i = 1:numel(keys)
    disp(['Key: ', keys{i}, ', Value: ', values{i}]);

    TITLE = values{i};
    filePath_orig = ['/home/boris/MATLAB/XR_SIM/res/' keys{i}];
    
    test_var = sprintf("%s", filePath_orig)
    filePath = filePath_orig; %%just some shenanigans to keep track f the original var without modifying more code
    
    fileNamePattern = TITLE + "_S%d_T400_FPS90_L1_BG%dNbg%d.csv"
    
    
    
    % Get a list of all files in the directory
    fileList = dir(filePath); % Update the directory path accordingly
    cd(filePath); %%enter the working directory
    
    % Initialize the outer struct to store the nested structs
    outerStruct = struct();
    
    % Iterate over each file in the directory
    for i = 1:numel(fileList)
        filename = fileList(i).name;
        
        % Match the filename pattern
        if(size(filename,2)>25)
    
            match = sscanf(filename, fileNamePattern);
            
            % Check if the filename matches the pattern
            if numel(match) == 3
                S = match(1);
                BG = match(2);
                Nbg = match(3);
    
                % if(S==1) 
                
                    % Create the nested struct field using the values of "BG" and "Nbg"
                    nestedPrefix = sprintf('BG_%d__Nbg_%d', BG, Nbg);
                    
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
        fig = figure;
        
        tilelay2 = tiledlayout(5,1, "TileSpacing","compact");
    
        a_name = sprintf("%s: Scenario %s ", TITLE, fields{k});
        set(fig, 'UserData', fields{k});
        title(tilelay2, a_name)
    
        % disp(sub_fields{1})
        for i=1:numel(sub_fields)
            data = outerStruct.(fields{k}).(sub_fields{i}); %% ONLY ONE SEED IN THIS CASE
            % head(outerStruct.(fields{k}).(sub_fields{i}))
            a = sprintf("%s",sub_fields{i});
            % title = a;
            
            nexttile
            
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
                histogram(data.load);
                ylim([0 300])
                title(a);
            end
            title(a)
            shg
        end
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
    
    % Loop through each figure and save it
    for i = 1:numel(figHandles)
        % Get the figure's title
        titleStr = get(figHandles(i), 'UserData');
        % disp("titstr" + titleStr)
    
        % Remove any invalid characters from the title
        filename = regexprep(titleStr, '[^\w\-_.() ]', '_');
        
        % disp("FN:" + filename )
        % Set the full file path for saving
        filePath = fullfile(savePath, [filename '.png']);
        filePath_fig = fullfile(sv_fig, [filename '.fig']);
    
        
        % Save the figure as PNG using the saveas function
        saveas(figHandles(i), filePath, 'png');
        saveas(figHandles(i), filePath_fig, 'fig');
    
    
    end
    close all
end
