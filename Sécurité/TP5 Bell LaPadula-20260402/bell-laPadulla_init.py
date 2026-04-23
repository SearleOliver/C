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
    def __init__(self, name: str, uid: str, gid: str, security_level: SecurityLevel):
        self.name = name
        self.uid = uid
        self.gid = gid
        self.security_level = security_level

class Object:
    def __init__(self, name: str, uid: str, gid: str, security_level: SecurityLevel):
        self.name = name
        self.uid = uid
        self.gid = gid
        self.security_level = security_level

class Controleur():

    def __init__(self, classification: Classification):
        super().__init__()
        self.classification = classification
        self.subjects: dict[str, Subject] = {}
        self.objects: dict[str, Object] = {}
        # acm[subject_name][object_name] = {"read", "write"}
        self.acm: dict[str, dict[str, set]] = {}

    def add_subject(self, subject: Subject):
        self.subjects[subject.name] = subject
        self.acm[subject.name] = {}

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
        # ss-property : no read up — level(subject) >= level(object)
        return subject.security_level.dominates(obj.security_level)

    def _mac_write_allowed(self, subject: Subject, obj: Object) -> bool:
        # *-property : no write down — level(object) >= level(subject)
        return obj.security_level.dominates(subject.security_level)

    def read(self, subject_name: str, object_name: str) -> bool:
        subject = self.subjects.get(subject_name)
        obj = self.objects.get(object_name)
        if subject is None:
            raise ValueError(f"Sujet inconnu : {subject_name!r}")
        if obj is None:
            raise ValueError(f"Objet inconnu : {object_name!r}")
        return self._dac_allowed(subject, obj, "read") and self._mac_read_allowed(subject, obj)

    def write(self, subject_name: str, object_name: str) -> bool:
        subject = self.subjects.get(subject_name)
        obj = self.objects.get(object_name)
        if subject is None:
            raise ValueError(f"Sujet inconnu : {subject_name!r}")
        if obj is None:
            raise ValueError(f"Objet inconnu : {object_name!r}")
        return self._dac_allowed(subject, obj, "write") and self._mac_write_allowed(subject, obj)
    
    def execute(self, subject_name: str,object_name: str, permissions: set, trust: bool):
        subject = self.subjects.get(subject_name)
        object = self.objects.get(object_name)
        if subject is None:
            raise ValueError(f"Sujet inconnu : {subject_name!r}")
        if object  is None:
            raise ValueError(f"Objet inconnu : {object_name!r}")
        if trust :
            self.grant(subject_name,object_name,permissions)
        else :
            self.grant(subject_name,object_name,permissions)

        


        


    
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
        ("alice", "rapport_rh",    "read"),   # ✅ DAC ok, SECRET >= CONF
        ("alice", "rapport_rh",    "write"),  # ✅ DAC ok, CONF >= SECRET → non : ❌ *-property
        ("alice", "dossier_top",   "write"),  # ✅ DAC ok, SECRET >= SECRET
        ("bob",   "rapport_rh",    "read"),   # ✅ DAC ok, CONF >= CONF
        ("bob",   "dossier_top",   "read"),   # ❌ pas de DAC sur dossier_top
        ("carol", "rapport_rh",    "read"),   # ❌ pas de DAC, PUBLIC < CONF
        ("carol", "note_publique", "write"),  # ✅ DAC ok, PUBLIC >= PUBLIC
        ("carol", "note_publique", "read"),   # ✅ DAC ok, PUBLIC >= PUBLIC
    ]

    print(f"{'Sujet':<8} {'Objet':<16} {'Op':<6} {'Résultat'}")
    print("-" * 42)
    for subject_name, object_name, op in tests:
        result = ctrl.read(subject_name, object_name) if op == "read" \
                 else ctrl.write(subject_name, object_name)
        print(f"{subject_name:<8} {object_name:<16} {op:<6} {'✅' if result else '❌'}")

