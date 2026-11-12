#!/usr/bin/env python3
import re
import sys


ROLE_PATTERN = re.compile(
    r"\[PID:(\d+)\] \[DEMARRAGE\] General \d+ "
    r"role=(COMMANDANT|LIEUTENANT) status=(LOYAL|TRAITRE)"
)
DECISION_PATTERN = re.compile(
    r"\[PID:(\d+)\] \[DECISION_FINALE\] Ma decision finale est : (Attaque|Retraite)"
)


def main():
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <journal> <n> <m>", file=sys.stderr)
        return 2

    log_path = sys.argv[1]
    n = int(sys.argv[2])
    m = int(sys.argv[3])

    roles = {}
    decisions = {}
    with open(log_path, encoding="utf-8") as log_file:
        for line in log_file:
            role_match = ROLE_PATTERN.search(line)
            if role_match:
                roles[int(role_match.group(1))] = (
                    role_match.group(2),
                    role_match.group(3),
                )
            decision_match = DECISION_PATTERN.search(line)
            if decision_match:
                decisions[int(decision_match.group(1))] = decision_match.group(2)

    loyal_lieutenants = {
        process_id
        for process_id, (role, status) in roles.items()
        if role == "LIEUTENANT" and status == "LOYAL"
    }
    if set(roles) != set(range(n)):
        print(f"ECHEC: identifiants de processus incomplets: {sorted(roles)}")
        return 1

    missing = loyal_lieutenants - decisions.keys()
    if missing:
        print(f"ECHEC: pas de decision pour les lieutenants loyaux {sorted(missing)}")
        return 1

    loyal_decisions = {decisions[process_id] for process_id in loyal_lieutenants}
    if 3 * m < n:
        if len(loyal_decisions) > 1:
            print(f"ECHEC: desaccord entre lieutenants loyaux: {sorted(loyal_decisions)}")
            return 1

        if roles.get(0) == ("COMMANDANT", "LOYAL") and loyal_decisions != {"Attaque"}:
            print("ECHEC: la validite n'est pas respectee avec un commandant loyal")
            return 1
        print("OK: accord des lieutenants loyaux; validite verifiee si le commandant est loyal.")
    else:
        print("HORS BORNE: aucune garantie d'accord/validite. "
              f"Decisions loyales observees: {sorted(loyal_decisions)}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
