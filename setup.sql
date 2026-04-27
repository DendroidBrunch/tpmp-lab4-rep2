CREATE TABLE IF NOT EXISTS Trawler (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    displacement REAL NOT NULL,
    build_date DATE NOT NULL
);

CREATE TABLE IF NOT EXISTS Bank (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS CrewMember (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    surname TEXT NOT NULL,
    position TEXT NOT NULL,
    hire_date DATE NOT NULL,
    birth_year INTEGER NOT NULL,
    trawler_id INTEGER,
    FOREIGN KEY (trawler_id) REFERENCES Trawler(id) ON DELETE SET NULL
);

CREATE TABLE IF NOT EXISTS Trip (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    trawler_id INTEGER NOT NULL,
    bank_id INTEGER NOT NULL,
    departure_date DATE NOT NULL,
    return_date DATE,
    FOREIGN KEY (trawler_id) REFERENCES Trawler(id) ON DELETE CASCADE,
    FOREIGN KEY (bank_id) REFERENCES Bank(id) ON DELETE RESTRICT
);

CREATE TABLE IF NOT EXISTS FishCatch (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    trip_id INTEGER NOT NULL,
    fish_name TEXT NOT NULL,
    quantity REAL NOT NULL,
    quality TEXT NOT NULL CHECK (quality IN ('высокое', 'среднее', 'низкое')),
    FOREIGN KEY (trip_id) REFERENCES Trip(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS Statistics (
    trawler_id INTEGER PRIMARY KEY,
    total_catch REAL DEFAULT 0,
    FOREIGN KEY (trawler_id) REFERENCES Trawler(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS Bonus (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    crew_id INTEGER NOT NULL,
    amount REAL NOT NULL,
    period_start DATE NOT NULL,
    period_end DATE NOT NULL,
    description TEXT,
    FOREIGN KEY (crew_id) REFERENCES CrewMember(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS Users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    password TEXT NOT NULL,
    role TEXT NOT NULL CHECK (role IN ('MANAGER', 'CREW')),
    crew_id INTEGER,
    FOREIGN KEY (crew_id) REFERENCES CrewMember(id) ON DELETE SET NULL
);

CREATE TRIGGER IF NOT EXISTS update_statistics_on_insert
AFTER INSERT ON FishCatch
BEGIN
    UPDATE Statistics SET total_catch = (
        SELECT COALESCE(SUM(fc.quantity), 0)
        FROM FishCatch fc
        JOIN Trip t ON fc.trip_id = t.id
        WHERE t.trawler_id = (SELECT trawler_id FROM Trip WHERE id = NEW.trip_id)
    ) WHERE trawler_id = (SELECT trawler_id FROM Trip WHERE id = NEW.trip_id);
END;

CREATE TRIGGER IF NOT EXISTS update_statistics_on_delete
AFTER DELETE ON FishCatch
BEGIN
    UPDATE Statistics SET total_catch = (
        SELECT COALESCE(SUM(fc.quantity), 0)
        FROM FishCatch fc
        JOIN Trip t ON fc.trip_id = t.id
        WHERE t.trawler_id = (SELECT trawler_id FROM Trip WHERE id = OLD.trip_id)
    ) WHERE trawler_id = (SELECT trawler_id FROM Trip WHERE id = OLD.trip_id);
END;

CREATE INDEX IF NOT EXISTS idx_trip_trawler ON Trip(trawler_id);
CREATE INDEX IF NOT EXISTS idx_trip_bank ON Trip(bank_id);
CREATE INDEX IF NOT EXISTS idx_trip_dates ON Trip(departure_date, return_date);
CREATE INDEX IF NOT EXISTS idx_fishcatch_trip ON FishCatch(trip_id);
CREATE INDEX IF NOT EXISTS idx_crew_trawler ON CrewMember(trawler_id);
CREATE INDEX IF NOT EXISTS idx_bonus_crew ON Bonus(crew_id);
CREATE INDEX IF NOT EXISTS idx_bonus_period ON Bonus(period_start, period_end);

CREATE VIEW IF NOT EXISTS TripSummary AS
SELECT 
    t.id as trip_id,
    tr.name as trawler_name,
    b.name as bank_name,
    t.departure_date,
    t.return_date,
    SUM(fc.quantity) as total_catch
FROM Trip t
JOIN Trawler tr ON t.trawler_id = tr.id
JOIN Bank b ON t.bank_id = b.id
LEFT JOIN FishCatch fc ON t.id = fc.trip_id
GROUP BY t.id;

INSERT OR IGNORE INTO Trawler (id, name, displacement, build_date) VALUES
    (1, 'Атлант', 2500.5, '2015-03-15'),
    (2, 'Тихий Океан', 3100.0, '2018-07-22'),
    (3, 'Северное Сияние', 1800.75, '2012-11-10');

INSERT OR IGNORE INTO Bank (id, name) VALUES
    (1, 'Северная банка'),
    (2, 'Центральная банка'),
    (3, 'Южная банка');

INSERT OR IGNORE INTO CrewMember (id, surname, position, hire_date, birth_year, trawler_id) VALUES
    (1, 'Иванов', 'капитан', '2010-01-15', 1975, 1),
    (2, 'Петров', 'боцман', '2012-05-20', 1980, 1),
    (3, 'Сидоров', 'капитан', '2015-03-10', 1970, 2),
    (4, 'Козлов', 'матрос', '2018-06-01', 1985, 2),
    (5, 'Михайлов', 'капитан', '2011-08-30', 1965, 3);

INSERT OR IGNORE INTO Users (username, password, role, crew_id) VALUES
    ('manager', '1ae76917f6dc38', 'MANAGER', NULL),
    ('ivanov', '1ae76917f6dc38', 'CREW', 1),
    ('petrov', '1ae76917f6dc38', 'CREW', 2),
    ('sidorov', '1ae76917f6dc38', 'CREW', 3);

INSERT OR IGNORE INTO Trip (id, trawler_id, bank_id, departure_date, return_date) VALUES
    (1, 1, 1, '2024-01-10', '2024-01-25'),
    (2, 1, 2, '2024-02-05', '2024-02-20'),
    (3, 2, 2, '2024-01-15', '2024-01-30'),
    (4, 2, 3, '2024-03-01', '2024-03-15'),
    (5, 3, 1, '2024-02-10', '2024-02-25');

INSERT OR IGNORE INTO FishCatch (trip_id, fish_name, quantity, quality) VALUES
    (1, 'Треска', 5000, 'высокое'),
    (1, 'Сельдь', 3000, 'среднее'),
    (2, 'Минтай', 8000, 'высокое'),
    (2, 'Камбала', 2000, 'низкое'),
    (3, 'Треска', 6000, 'среднее'),
    (3, 'Сельдь', 4000, 'высокое'),
    (4, 'Минтай', 7000, 'низкое'),
    (5, 'Камбала', 3500, 'среднее');

INSERT OR IGNORE INTO Statistics (trawler_id, total_catch) VALUES
    (1, (SELECT COALESCE(SUM(fc.quantity), 0) FROM FishCatch fc JOIN Trip t ON fc.trip_id = t.id WHERE t.trawler_id = 1)),
    (2, (SELECT COALESCE(SUM(fc.quantity), 0) FROM FishCatch fc JOIN Trip t ON fc.trip_id = t.id WHERE t.trawler_id = 2)),
    (3, (SELECT COALESCE(SUM(fc.quantity), 0) FROM FishCatch fc JOIN Trip t ON fc.trip_id = t.id WHERE t.trawler_id = 3));
