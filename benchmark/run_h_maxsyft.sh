cd ./../build/bin

ulimit -v $((8 * 1024 * 1024)) # Set memory limit to 8 GB

for i in {1..15}; do
timeout 1000s ./MaxSyft -d ./../../benchmark/tire_domain.pddl -p ./../../benchmark/tire_problem.pddl -g "./../../benchmark/maxsyft_h_tire_${i}.ltlf" -o ./../../benchmark/maxsyft_h_results.csv
sleep 5
done
