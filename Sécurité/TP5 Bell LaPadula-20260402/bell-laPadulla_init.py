"""
@author Yannick Chevalier
@date 2026
"""

class Contexts:
    contexts=set([]) # type set
    def __init__(self,ctxts=[]):
        self.contexts=set(ctxts)
    def add(self,ctxt):
        self.contexts.add(ctxt)
    def printContexts(self): # pour debug
        print(self.contexts)
    def compare(self,aContext):
        lowerOrEq=(self.contexts <= aContext.contexts)
        greaterOrEq=(self.contexts >= aContext.contexts)
        if lowerOrEq and greaterOrEq:
            return 0 # equal
        elif lowerOrEq:
            return -1
        elif greaterOrEq:
            return 1
        else:
            return None


class SecurityLevel:
    hierarchy=None
    contexts=None
    level=None
    def __init__(self,classification=hierarchy,ctxts=[],level=None):
        self.contexts=Contexts(ctxts)
        self.level=level
        self.hierarchy=classification
    def dominates(self, aSecurityLevel):
        cmp = self.compare(aSecurityLevel)
        return cmp is not None and cmp >= 0
    def compare(self,aSecurityLevel):
        if not ( self.hierarchy == aSecurityLevel.hierarchy):
            return None
        ctxtcmp=self.contexts.compare(aSecurityLevel.contexts)
        levelcmp=self.hierarchy.compare(self.level,aSecurityLevel.level)
        # if either is incomparable, the result is incomparable
        if ctxtcmp is None or levelcmp is None:
            return None
        # if they are both non-zero but of a different sign, the result is None
        if ctxtcmp * levelcmp < 0:
            return None
        # if they agree (or equal)...
        return ctxtcmp+levelcmp
    def printLevel(self):
        print(f'hierarchy={self.hierarchy}, contexts={self.contexts.printContexts()}, level={self.level}')

class Classification:
    # classification is a dictionary that stores, for each security
    # level, all the levels strictly below it
    classification=None
    def __init__(self):
        self.classification=dict([])
    def addClassificationLevel(self,level,above=[]):
        # adds a level above those listed
        s= set(above)
        for lvl in above:
            s_level=self.classification.get(lvl)
            if s_level is not None:
                s=s.union(s_level)
        if level in s:
            raise Exception (f'Error, level {level} is already defined and below one of the levels it should be above')
        self.classification[level]=s
    def securityLevel(self,ctxts=[],level=None):
        if level not in self.classification:
            raise Exception(f'Level {level} is not known in this classification')
        return SecurityLevel(classification=self,ctxts=ctxts,level=level)
    def compare(self,level1,level2):
        if level1 == level2:
            return 0
        s1 = self.classification.get(level1)
        s2 = self.classification.get(level2)
        if s1 is None:
            raise Exception (f'Error, level {level1} is not defined in this classification')
        if s2 is None:
            raise Exception (f'Error, level {level2} is not defined in this classification')
        if level1 == level2:
            return 0
        if level1 in s2:
            return -1
        if level2 in s1:
            return 1
        return None

class Subject:
    def __init__(self, name: str, uid: str, gid: str, security_level: SecurityLevel, max_level: SecurityLevel = None):
        self.name = name
        self.uid = uid
        self.gid = gid
        self.security_level = security_level
        self.max_level = max_level if max_level is not None else security_level

class Object:
    def __init__(self, name: str, uid: str, gid: str, security_level: SecurityLevel):
        self.name = name
        self.uid = uid
        self.gid = gid
        self.security_level = security_level

class Controleur:

    def __init__(self, classification: Classification):
        self.classification = classification
        self.subjects: dict[str, Subject] = {}
        self.objects:  dict[str, Object]  = {}
        self.acm:      dict[str, dict[str, set]] = {}
        self.open_files: dict[str, dict[str, set]] = {}

    def add_subject(self, subject: Subject):
        self.subjects[subject.name] = subject
        self.acm[subject.name] = {}
        self.open_files[subject.name] = {"read": set(), "write": set()}

    def add_object(self, obj: Object):
        self.objects[obj.name] = obj

    def grant(self, subject_name: str, object_name: str, permissions: set):
        if subject_name not in self.subjects:
            raise ValueError(f"Sujet inconnu : {subject_name!r}")
        if object_name not in self.objects:
            raise ValueError(f"Objet inconnu : {object_name!r}")
        self.acm[subject_name][object_name] = permissions

    def _dac_allowed(self, subject: Subject, obj: Object, operation: str) -> bool:
        return operation in self.acm.get(subject.name, {}).get(obj.name, set())

    def _mac_read_allowed(self, subject: Subject, obj: Object) -> bool:
        return subject.security_level.dominates(obj.security_level)

    def _mac_write_allowed(self, subject: Subject, obj: Object) -> bool:
        return obj.security_level.dominates(subject.security_level)

    def read(self, subject_name: str, object_name: str) -> bool:
        subject = self.subjects.get(subject_name)
        obj     = self.objects.get(object_name)
        if subject is None:
            raise ValueError(f"Sujet inconnu : {subject_name!r}")
        if obj is None:
            raise ValueError(f"Objet inconnu : {object_name!r}")
        
        allowed = self._dac_allowed(subject, obj, "read") and self._mac_read_allowed(subject, obj)
        if allowed:
            self.open_files[subject_name]["read"].add(object_name)
        return allowed

    def write(self, subject_name: str, object_name: str) -> bool:
        subject = self.subjects.get(subject_name)
        obj     = self.objects.get(object_name)
        if subject is None:
            raise ValueError(f"Sujet inconnu : {subject_name!r}")
        if obj is None:
            raise ValueError(f"Objet inconnu : {object_name!r}")
        
        allowed = self._dac_allowed(subject, obj, "write") and self._mac_write_allowed(subject, obj)
        if allowed:
            self.open_files[subject_name]["write"].add(object_name)
        return allowed

    def close(self, subject_name: str, object_name: str, operation: str):
        if subject_name not in self.open_files:
            raise ValueError(f"Sujet inconnu : {subject_name!r}")
        
        if operation not in ("read", "write"):
            raise ValueError(f"Opération invalide : {operation!r}")
        self.open_files[subject_name][operation].discard(object_name)

    def execute(self, subject_name: str, object_name: str, new_subject_name: str, requested_level: SecurityLevel, trust: bool) -> bool:
        subject = self.subjects.get(subject_name)
        obj     = self.objects.get(object_name)
        if subject is None:
            raise ValueError(f"Sujet inconnu : {subject_name!r}")
        if obj is None:
            raise ValueError(f"Objet inconnu : {object_name!r}")
        
        if not self._dac_allowed(subject, obj, "execute"):
            return False
        if trust:
            new_level = requested_level
        else:
            if not subject.security_level.dominates(requested_level):
                return False
            new_level = requested_level
        new_subject = Subject(new_subject_name, uid=subject.uid, gid=subject.gid, security_level=new_level)
        self.add_subject(new_subject)
        return True

    def kill(self, subject_name: str, target_name: str) -> bool:
        subject = self.subjects.get(subject_name)
        target  = self.subjects.get(target_name)
        if subject is None:
            raise ValueError(f"Sujet inconnu : {subject_name!r}")
        if target is None:
            raise ValueError(f"Sujet inconnu : {target_name!r}")

        if subject_name != target_name and subject.uid != target.uid:
            return False

        # cascade uniquement si le sujet principal se suicide (uid == name)
        if subject_name == target_name and target.uid == target.name:
            to_kill = [
                name for name, s in self.subjects.items()
                if s.uid == target.uid
            ]
        else:
            to_kill = [target_name]

        for name in to_kill:
            del self.subjects[name]
            del self.acm[name]
            del self.open_files[name]

        return True

    def touch(self, subject_name: str, object_name: str, gid: str) -> bool:
        subject = self.subjects.get(subject_name)
        if subject is None:
            raise ValueError(f"Sujet inconnu : {subject_name!r}")
        
        # existe déjà
        if object_name in self.objects:
            return False  
        new_obj = Object(object_name, uid=subject.uid, gid=gid, security_level=subject.security_level)
        self.add_object(new_obj)
        # l'auteur à tout les droits sur son objet
        self.grant(subject_name, object_name, {"read", "write", "execute"})
        return True

    def rm(self, subject_name: str, object_name: str) -> bool:
        subject = self.subjects.get(subject_name)
        obj     = self.objects.get(object_name)
        if subject is None:
            raise ValueError(f"Sujet inconnu : {subject_name!r}")
        
        if obj is None:
            return False
        # seul le propriétaire peut supprimer
        if subject.uid != obj.uid:
            return False  
        # ferme tous les accès ouverts
        for s_name in self.open_files:
            self.open_files[s_name]["read"].discard(object_name)
            self.open_files[s_name]["write"].discard(object_name)
        # retire l'objet de la matrice ACM
        for s_name in self.acm:
            self.acm[s_name].pop(object_name, None)
        del self.objects[object_name]
        return True

    def chmod(self, subject_name: str, object_name: str, target_subject: str, new_permissions: set) -> bool:
        subject = self.subjects.get(subject_name)
        obj     = self.objects.get(object_name)
        if subject is None:
            raise ValueError(f"Sujet inconnu : {subject_name!r}")
        if obj is None:
            raise ValueError(f"Objet inconnu : {object_name!r}")
        if target_subject not in self.subjects:
            raise ValueError(f"Sujet inconnu : {target_subject!r}")
         # seul le propriétaire peut modifier les droits
        if subject.uid != obj.uid:
            return False 
        self.grant(target_subject, object_name, new_permissions)
        return True

    def chown(self, subject_name: str, object_name: str, new_uid: str, new_gid: str) -> bool:
        """Transfère la propriété d'un objet. Seul le propriétaire actuel peut le faire."""
        subject = self.subjects.get(subject_name)
        obj     = self.objects.get(object_name)
        if subject is None:
            raise ValueError(f"Sujet inconnu : {subject_name!r}")
        if obj is None:
            raise ValueError(f"Objet inconnu : {object_name!r}")
        if subject.uid != obj.uid:
            return False
        obj.uid = new_uid
        obj.gid = new_gid
        return True
    
    def change_level(self, subject_name: str, requested_level: SecurityLevel) -> bool:
        subject = self.subjects.get(subject_name)
        if subject is None:
            raise ValueError(f"Sujet inconnu : {subject_name!r}")

        # niveau demandé doit être <= niveau maximal du sujet
        if not subject.max_level.dominates(requested_level):
            return False

        # ss-property : nouveau niveau doit dominer tous les objets ouverts en lecture
        for obj_name in self.open_files[subject_name]["read"]:
            obj = self.objects.get(obj_name)
            if obj and not requested_level.dominates(obj.security_level):
                return False

        # *-property : niveau de chaque objet ouvert en écriture doit dominer le nouveau niveau
        for obj_name in self.open_files[subject_name]["write"]:
            obj = self.objects.get(obj_name)
            if obj and not obj.security_level.dominates(requested_level):
                return False

        subject.security_level = requested_level
        return True

        

if __name__ == "__main__":

    classif = Classification()
    classif.addClassificationLevel("PUBLIC")
    classif.addClassificationLevel("CONFIDENTIEL", above=["PUBLIC"])
    classif.addClassificationLevel("SECRET",       above=["CONFIDENTIEL"])

    lvl_public = classif.securityLevel(level="PUBLIC")
    lvl_conf   = classif.securityLevel(level="CONFIDENTIEL")
    lvl_secret = classif.securityLevel(level="SECRET")

    ctrl = Controleur(classif)

    alice = Subject("alice", uid="alice", gid="rh",      security_level=lvl_secret)
    bob   = Subject("bob",   uid="bob",   gid="finance",  security_level=lvl_conf)
    carol = Subject("carol", uid="carol", gid="rh",       security_level=lvl_public)

    f1 = Object("rapport_rh",    uid="alice", gid="rh",      security_level=lvl_conf)
    f2 = Object("note_publique", uid="carol", gid="rh",      security_level=lvl_public)
    f3 = Object("dossier_top",   uid="alice", gid="finance",  security_level=lvl_secret)

    for s in (alice, bob, carol): ctrl.add_subject(s)
    for o in (f1, f2, f3):        ctrl.add_object(o)

    # Matrice DAC
    ctrl.grant("alice", "rapport_rh",    {"read", "write"})
    ctrl.grant("alice", "dossier_top",   {"read", "write"})
    ctrl.grant("bob",   "rapport_rh",    {"read"})
    ctrl.grant("carol", "note_publique", {"read", "write"})

    tests = [
    ("alice", "rapport_rh",    "read"),   # DAC ok, SECRET >= CONF
    ("alice", "rapport_rh",    "write"),  # *-property : CONF < SECRET
    ("alice", "dossier_top",   "write"),  # DAC ok, SECRET >= SECRET
    ("bob",   "rapport_rh",    "read"),   # DAC ok, CONF >= CONF
    ("bob",   "dossier_top",   "read"),   # pas de DAC sur dossier_top
    ("carol", "rapport_rh",    "read"),   # pas de DAC, PUBLIC < CONF
    ("carol", "note_publique", "write"),  # DAC ok, PUBLIC >= PUBLIC
    ("carol", "note_publique", "read"),   # DAC ok, PUBLIC >= PUBLIC
    ]

    print(f"{'Sujet':<8} {'Objet':<16} {'Op':<6} {'Résultat'}")
    print("-" * 42)
    for subject_name, object_name, op in tests:
        result = ctrl.read(subject_name, object_name) if op == "read" \
                 else ctrl.write(subject_name, object_name)
        print(f"{subject_name:<8} {object_name:<16} {op:<6} {'OUI' if result else 'NON'}")

    # grant d'exécution
    ctrl.grant("alice", "dossier_top",   {"read", "write", "execute"})
    ctrl.grant("bob",   "rapport_rh",    {"read", "execute"})

    print("\n── Tests execute ──────────────────────────────────")
    ok = ctrl.execute("alice", "dossier_top", "proc_alice", lvl_secret, trust=True)
    print(f"alice execute dossier_top trusted  : {'OUI' if ok else 'NON'}")

    ok = ctrl.execute("bob", "rapport_rh", "proc_bob", lvl_conf, trust=False)
    print(f"bob execute rapport_rh non-trusted CONF : {'OUI' if ok else 'NON'}")

    ok = ctrl.execute("bob", "rapport_rh", "proc_bob2", lvl_secret, trust=False)
    print(f"bob execute rapport_rh non-trusted SECRET : {'OUI' if ok else 'NON'}")

    print("\n── Tests touch / rm / chmod / chown ───────────────")

    ok = ctrl.touch("alice", "nouveau_fichier", "rh")
    print(f"alice touch nouveau_fichier : {'OUI' if ok else 'NON'}")

    ok = ctrl.read("alice", "nouveau_fichier")
    print(f"alice read  nouveau_fichier : {'OUI' if ok else 'NON'}")
    print(f"fichiers ouverts par alice : {ctrl.open_files['alice']}")

    ctrl.close("alice", "nouveau_fichier", "read")
    print(f"après close : {ctrl.open_files['alice']}")

    ok = ctrl.chmod("alice", "nouveau_fichier", "bob", {"read"})
    print(f"alice chmod bob sur nouveau_fichier : {'OUI' if ok else 'NON'}")

    ok = ctrl.read("bob", "nouveau_fichier")
    print(f"bob read  nouveau_fichier (CONF < SECRET) : {'OUI' if ok else 'NON'}")

    ok = ctrl.chown("alice", "nouveau_fichier", new_uid="carol", new_gid="rh")
    print(f"alice chown → carol : {'OUI' if ok else 'NON'}")

    ok = ctrl.rm("alice", "nouveau_fichier")
    print(f"alice rm nouveau_fichier (plus proprio) : {'OUI' if ok else 'NON'}")

    ok = ctrl.rm("carol", "nouveau_fichier")
    print(f"carol rm nouveau_fichier : {'OUI' if ok else 'NON'}")
    print(f"objets restants : {list(ctrl.objects.keys())}")

    print("\n── Tests change_level ─────────────────────────────")
    ctrl.close("bob", "rapport_rh", "read")

    bob.max_level = lvl_secret
    print(f"Le plafond de Bob deviens Secret.")

    # Bob passe de conf à secret
    ok = ctrl.change_level("bob", lvl_secret)
    print(f"bob change_level SECRET   : {'OUI' if ok else 'NON'} → {bob.security_level.level}")

    # Bob passe de secret à public
    ok = ctrl.change_level("bob", lvl_public)
    print(f"bob change_level PUBLIC   : {'OUI' if ok else 'NON'} → {bob.security_level.level}")

    # Bob reviens à confidentiel
    ok = ctrl.change_level("bob", lvl_conf)
    print(f"bob change_level  CONFIDENTIEL  : {'OUI' if ok else 'NON'} → {bob.security_level.level}")
    ctrl.grant("bob", "rapport_rh", {"read"})
    ctrl.read("bob", "rapport_rh")
    print(f"bob a ouvert : {ctrl.open_files['bob']}")
    # Bob essaye de descendre à public avec un fichier conf ouvert
    ok = ctrl.change_level("bob", lvl_public)
    print(f"bob change_level PUBLIC (rapport_rh ouvert en lecture) : {'OUI' if ok else 'NON'}")

    # Bob essaye après avoir fermer le fichier
    ctrl.close("bob", "rapport_rh", "read")
    ok = ctrl.change_level("bob", lvl_public)
    print(f"bob change_level PUBLIC (après close)  : {'OUI' if ok else 'NON'} → {bob.security_level.level}")

    # Carol essaye de monter au niv secret
    ok = ctrl.change_level("carol", lvl_secret)
    print(f"carol change_level SECRET (plafond PUBLIC) : {'OUI' if ok else 'NON'}")

    print("\n── Tests kill ─────────────────────────────────────")
    print(f"sujets avant kill : {list(ctrl.subjects.keys())}")

    # Alice a le droit de se kill
    ok = ctrl.kill("alice", "alice")
    print(f"alice kill alice : {'OUI' if ok else 'NON'}")

    # Bob a le droit de kill son processus
    ok = ctrl.kill("bob", "proc_bob")
    print(f"bob kill proc_bob : {'OUI' if ok else 'NON'}")

    # Bob a le droit de kill son processus
    ok = ctrl.kill("bob", "carol")
    print(f"bob kill carol : {'OUI' if ok else 'NON'}")

    print(f"sujets après kills : {list(ctrl.subjects.keys())}")

