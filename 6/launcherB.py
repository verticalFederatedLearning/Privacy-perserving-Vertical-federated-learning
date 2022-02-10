#!/usr/bin/env python3

import argparse
import logging
import os

def _run_experiment():
    level = logging.INFO
    if "RANK" in os.environ and os.environ["RANK"] != "0":
        level = logging.CRITICAL
    logging.getLogger().setLevel(level)
    logging.basicConfig(
        level=level,
        format="%(asctime)s - %(process)d - %(name)s - %(levelname)s - %(message)s",
    )
    from mpc_autograd import run_mpc_autograd

    run_mpc_autograd(
        num_epochs=10,
        learning_rate=0.01,
        batch_size=100,
        print_freq=5,
        num_samples=1000,
    )

if __name__ == "__main__":
    custom_env = {
        "WORLD_SIZE":"2",
        "RANK":"1",
        "RENDEZVOUS":"tcp://127.0.0.1:10010",
    }

    for k,v in custom_env.items():
        os.environ[k] = v
    _run_experiment()

