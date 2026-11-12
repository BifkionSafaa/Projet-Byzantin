import re
import os
import graphviz

LOG_FILE = "logs/trace_globale.log"

def generer_graphviz():
    if not os.path.exists(LOG_FILE):
        print(f"⚠️ Fichier {LOG_FILE} introuvable.")
        return

    # Création du graphe orienté avec le moteur "circo" (disposition en cercle)
    # On utilise xlabel pour contourner le bug des labels sur flèches courbées
    dot = graphviz.Digraph(comment='Réseau Byzantin', format='png', engine='circo')
    
    dot.attr('node', shape='circle', style='filled', color='lightblue', fontname='Arial', fontsize='14', width='1.5')
    dot.attr('edge', fontname='Arial', fontsize='12', penwidth='2')
    
    # Nœuds
    dot.node('0', 'Commandant', fillcolor='lightcoral') 
    dot.node('1', 'L1')
    dot.node('2', 'L2')
    dot.node('3', 'L3')

    # Patterns pour lire les logs
    pattern_tour1 = re.compile(r"\[PID:(\d+)\] \[RECEPTION_TOUR1\] J'ai recu du Commandant : (\w+)")
    pattern_tour2 = re.compile(r"\[PID:(\d+)\] \[TOUR2_ENVOI\] Envoye '(\d+):(\w+)' au lieutenant (\d+)")
    
    with open(LOG_FILE, "r") as f:
        for ligne in f:
            # --- TOUR 1 ---
            match_t1 = pattern_tour1.search(ligne)
            if match_t1:
                dest = match_t1.group(1)
                msg = match_t1.group(2)
                couleur = 'green' if msg == 'Attaque' else 'red'
                dot.edge('0', dest, xlabel=msg, color=couleur)

            # --- TOUR 2 ---
            match_t2 = pattern_tour2.search(ligne)
            if match_t2:
                exp = match_t2.group(1)
                msg = match_t2.group(3)
                dest = match_t2.group(4)
                
                # On force le dessin des deux flèches (A -> B et B -> A)
                # L'utilisation de xlabel évite le crash de Graphviz
                dot.edge(exp, dest, xlabel=msg, color='blue')

    # Sauvegarde
    os.makedirs("logs", exist_ok=True)
    chemin_sortie = "logs/graphe_reseau"
    dot.render(chemin_sortie, cleanup=True)
    print(f"✅ Graphique généré avec succès : {chemin_sortie}.png")

if __name__ == "__main__":
    print("📊 Génération du graphe avec Graphviz...")
    generer_graphviz()