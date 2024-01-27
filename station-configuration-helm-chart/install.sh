helm upgrade --install super-simple-radio-station-config-api . --atomic -f values.yaml -f secrets.yaml || kubectl get events --sort-by='{.lastTimestamp}'
# || ... -> on failure, show logs