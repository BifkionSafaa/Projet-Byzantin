import os
import re

import graphviz


LOG_FILE = "logs/trace_globale.log"
ROLE_PATTERN = re.compile(
    r"\[PID:(\d+)\] \[DEMARRAGE\] General \d+ "
    r"role=(COMMANDANT|LIEUTENANT) status=(LOYAL|TRAITRE)"
)
OM_SEND_PATTERN = re.compile(
    r"\[PID:(\d+)\] \[OM_ENVOI\] round=(\d+) path=([^ ]+) "
    r"value=(Attaque|Retraite) dest=(\d+)"
)
LEGACY_TOUR1_PATTERN = re.compile(
    r"\[PID:(\d+)\] \[RECEPTION_TOUR1\] J'ai recu du Commandant : (\w+)"
)
LEGACY_TOUR2_PATTERN = re.compile(
    r"\[PID:(\d+)\] \[TOUR2_ENVOI\] (?:Envoye|Envoie) "
    r"'(\d+):(\w+)' au lieutenant (\d+)"
)


def generer_graphviz():
    if not os.path.exists(LOG_FILE):
        print(f"Fichier {LOG_FILE} introuvable.")
        return

    dot = graphviz.Digraph(comment="Reseau Byzantin", format="png", engine="circo")
    dot.attr(
        "node",
        shape="circle",
        style="filled",
        color="lightblue",
        fontname="Arial",
        fontsize="12",
        width="1.4",
    )
    dot.attr("edge", fontname="Arial", fontsize="9", penwidth="1.5")

    nodes = {}
    sends = []
    with open(LOG_FILE, encoding="utf-8") as log_file:
        for line in log_file:
            role_match = ROLE_PATTERN.search(line)
            if role_match:
                node_id, role, status = role_match.groups()
                nodes[node_id] = (role, status)
                continue

            send_match = OM_SEND_PATTERN.search(line)
            if send_match:
                sender, round_number, path, value, destination = send_match.groups()
                sends.append((sender, destination, round_number, path, value))
                continue

            legacy_tour1 = LEGACY_TOUR1_PATTERN.search(line)
            if legacy_tour1:
                destination, value = legacy_tour1.groups()
                sends.append(("0", destination, "1", "-", value))
                continue

            legacy_tour2 = LEGACY_TOUR2_PATTERN.search(line)
            if legacy_tour2:
                sender, _message_sender, value, destination = legacy_tour2.groups()
                sends.append((sender, destination, "2", "-", value))

    for sender, destination, *_ in sends:
        nodes.setdefault(sender, ("LIEUTENANT", "LOYAL"))
        nodes.setdefault(destination, ("LIEUTENANT", "LOYAL"))
    nodes.setdefault("0", ("COMMANDANT", "LOYAL"))

    for node_id in sorted(nodes, key=int):
        role, status = nodes[node_id]
        label = "Commandant" if role == "COMMANDANT" else f"L{node_id}"
        if status == "TRAITRE":
            label += "\nTRAITRE"
        dot.node(
            node_id,
            label,
            fillcolor="lightcoral" if status == "TRAITRE" else "lightblue",
        )

    for sender, destination, round_number, path, value in sends:
        color = "green" if value == "Attaque" else "red"
        dot.edge(
            sender,
            destination,
            xlabel=f"r{round_number} {path}: {value}",
            color=color,
        )

    os.makedirs("logs", exist_ok=True)
    output_path = "logs/graphe_reseau"
    dot.render(output_path, cleanup=True)
    print(f"Graphe genere : {output_path}.png")


if __name__ == "__main__":
    generer_graphviz()
