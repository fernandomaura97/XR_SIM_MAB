clear all
close all
clc

apath = "/home/boris/Documents/cost_rl/Results/csv/TUNING_MAX";
pit = {"Q-N2_"};

figure();
tiledlayout(4,3);

title(pit);
fileNamePattern = [ pit + "_S%d_T400_FPS90_L1_BG20Nbg3_a%f_g%f_Tu%f.csv"];

options = 'hist';

contents = dir(apath);
outerStruct = struct();

% Iterate through the contents
for i = 1:numel(contents)
    item = contents(i);
    % Check if the item is a folder
    if item.isdir && ~strcmp(item.name, '.') && ~strcmp(item.name, '..')

        folderName = fullfile(apath, item.name);
        cd(folderName);
        files = dir(fullfile(folderName, '*.*'));
        fileList = files(~[files.isdir]); % Exclude subfolders

        for i = 1:numel(fileList)
            fn = fileList(i).name;

            if size(fn,2)>10

                matches = sscanf(fn, fileNamePattern);
                if(numel(matches))== 4
                    S = matches(1);
                    alpha = matches(2);
                    gamma = matches(3);
                    T_u = matches(4);

                    nestedPrefix = sprintf("a%.0f", 10*alpha);

                    if isfield(outerStruct, nestedPrefix)
                        alphaStruct = outerStruct.(nestedPrefix);
                    else
                        alphaStruct = struct();
                    end

                    gammaPrefix = sprintf("g%.0f", 10*gamma);

                    if isfield(alphaStruct, gammaPrefix)
                        gammaStruct = alphaStruct.(gammaPrefix);
                    else
                        gammaStruct = struct();
                    end

                    prefix = sprintf("S%d", S);

                    if isfield(gammaStruct, prefix)
                        % Append the table to the existing field within the gamma struct
                        gammaStruct.(prefix) = [gammaStruct.(prefix); readtable(fn)];
                    else
                        % Create a new field within the gamma struct and assign the table
                        gammaStruct.(prefix) = readtable(fn);
                    end

                    alphaStruct.(gammaPrefix) = gammaStruct;
                    outerStruct.(nestedPrefix) = alphaStruct;
                end
            end
        end
    end
end

% PLOTS FROM STRUCT
fields = fieldnames(outerStruct);

for i = 1:numel(fields)

    sub_fields = fieldnames(outerStruct.(fields{i}));

    for k = 1:numel(sub_fields)
        TITLE = [fields{i}, sub_fields{k}];
        data_v = {};
        a = TITLE;
        nexttile;

        subsub_fields = fieldnames(outerStruct.(fields{i}).(sub_fields{k}));

        for i2=1:numel(subsub_fields) % seeds
            hold on
            data = outerStruct.(fields{i}).(sub_fields{k}).(subsub_fields{i2});

            if strcmp(options, 'hist')
                % Set the desired number of bins and the range of the x-axis
                numBins = 10;
                xMin = 0;
                xMax = 100E6;

                % Compute the common bin edges
                binEdges = linspace(xMin, xMax, numBins + 1);

                % Create the histogram with the specified bin edges
                histogram(data.load, binEdges, 'FaceAlpha', 0.15);

                ylim([0 300]);
                xlim([xMin xMax]);

                pat = "a%dg%d";
                match = sscanf(TITLE, pat)
                alpha = match(1)/10
                gamma = match(2)/10
                supertitle = sprintf('α = %.1f, γ = %.1f', alpha, gamma);
                title(supertitle);

            elseif strcmp(options, 'cdf')
                cdfplot(data.load);
                title(TITLE);
            end

            hold off;
            shg;
        end
    end
end

